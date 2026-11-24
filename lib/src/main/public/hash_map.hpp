#pragma once
#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include "detail/iterator/hash_iterator.hpp"
#include "detail/node/list_node.hpp"
#include "detail/utils/constants.hpp"
#include "detail/utils/conversion.hpp"
#include "detail/utils/exceptions.hpp"
#include "detail/utils/printing.hpp"

namespace ds
{
	// Hash map with separate chaining. The bucket count is a power of two, so a bucket is the hash masked by
	// the count; the table doubles once the load factor exceeds its maximum.
	template <class key_type, class mapped_type, class hasher = std::hash<key_type>>
	class hash_map
	{
	public:
		using value_type = std::pair<const key_type, mapped_type>;
		using size_type = std::size_t;

	private:
		using node_type = detail::list_node<value_type>;

	public:
		using iterator = detail::hash_iterator<node_type, value_type>;
		using const_iterator = detail::hash_iterator<node_type, const value_type>;

		// Constructors and destructor. A repeated source key keeps the last mapped value.
		~hash_map();
		hash_map() noexcept(std::is_nothrow_default_constructible_v<hasher>);
		hash_map(std::initializer_list<value_type> initial);
		hash_map(const value_type* source, size_type count);
		hash_map(const hash_map& other);
		hash_map(hash_map&& other) noexcept(std::is_nothrow_move_constructible_v<hasher>);

		// Modifier methods.
		hash_map& operator=(const hash_map& other);
		hash_map& operator=(hash_map&& other) noexcept(std::is_nothrow_move_assignable_v<hasher>);
		hash_map& clear() noexcept;

		// Specific methods. Inserting a present key replaces its value; removing a missing key changes nothing.
		hash_map& insert(key_type key, mapped_type mapped);
		hash_map& remove(const key_type& key);

		// Constant methods.
		bool operator==(const hash_map& other) const;
		bool operator!=(const hash_map& other) const;
		size_type size() const noexcept;
		bool empty() const noexcept;
		void print() const;
		mapped_type& get(const key_type& key);
		const mapped_type& get(const key_type& key) const;
		bool contains(const key_type& key) const;

		// Iterator methods. Keys are read-only, mapped values are writable through a mutable iterator.
		iterator begin() noexcept;
		iterator end() noexcept;
		const_iterator begin() const noexcept;
		const_iterator end() const noexcept;

		// Friend functions.
		friend std::ostream& operator<<(std::ostream& out, const hash_map& map)
		{
			const auto stream_pair = [](std::ostream& target, const value_type& entry) { target << entry.first << ": " << entry.second; };
			return detail::write_elements(out, map, stream_pair);
		}

		friend std::unique_ptr<std::pair<key_type, mapped_type>[]> convert(const hash_map& map)
		{
			return detail::copy_elements<std::pair<key_type, mapped_type>>(map, map.length);
		}

	private:
		node_type** buckets = nullptr;
		size_type bucket_count = 0;
		size_type length = 0;
		hasher hashing{};

		size_type bucket_of(const key_type& key) const;
		node_type* find(const key_type& key) const;
		void grow_if_needed();
		void rehash(size_type replacement_count);
		void copy_chains(const hash_map& other);
		void release() noexcept;
		void steal(hash_map& other) noexcept;
		void swap(hash_map& other) noexcept;
	};

	//------------------------------------------------
	// Constructors and destructor.

	template <class key_type, class mapped_type, class hasher>
	hash_map<key_type, mapped_type, hasher>::~hash_map()
	{
		release();
	}

	template <class key_type, class mapped_type, class hasher>
	hash_map<key_type, mapped_type, hasher>::hash_map() noexcept(std::is_nothrow_default_constructible_v<hasher>) {}

	template <class key_type, class mapped_type, class hasher>
	hash_map<key_type, mapped_type, hasher>::hash_map(std::initializer_list<value_type> initial)
		: hash_map(initial.begin(), initial.size()) {}

	template <class key_type, class mapped_type, class hasher>
	hash_map<key_type, mapped_type, hasher>::hash_map(const value_type* source, size_type count) : hash_map()
	{
		for (size_type index = 0; index < count; ++index)
			insert(source[index].first, source[index].second);
	}

	template <class key_type, class mapped_type, class hasher>
	hash_map<key_type, mapped_type, hasher>::hash_map(const hash_map& other) : hashing(other.hashing)
	{
		try
		{
			copy_chains(other);
		}
		catch (...)
		{
			release();
			throw;
		}
	}

	template <class key_type, class mapped_type, class hasher>
	hash_map<key_type, mapped_type, hasher>::hash_map(hash_map&& other) noexcept(std::is_nothrow_move_constructible_v<hasher>)
		: hashing(std::move(other.hashing))
	{
		steal(other);
	}

	//------------------------------------------------
	// Modifier methods.

	template <class key_type, class mapped_type, class hasher>
	hash_map<key_type, mapped_type, hasher>& hash_map<key_type, mapped_type, hasher>::operator=(const hash_map& other)
	{
		hash_map copy(other);
		swap(copy);
		return *this;
	}

	template <class key_type, class mapped_type, class hasher>
	hash_map<key_type, mapped_type, hasher>& hash_map<key_type, mapped_type, hasher>::operator=(hash_map&& other) noexcept(std::is_nothrow_move_assignable_v<hasher>)
	{
		if (this == &other)
			return *this;

		release();
		steal(other);
		hashing = std::move(other.hashing);
		return *this;
	}

	template <class key_type, class mapped_type, class hasher>
	hash_map<key_type, mapped_type, hasher>& hash_map<key_type, mapped_type, hasher>::clear() noexcept
	{
		for (size_type bucket = 0; bucket < bucket_count; ++bucket)
		{
			node_type* current = std::exchange(buckets[bucket], nullptr);
			while (current != nullptr)
				delete std::exchange(current, current->next);
		}

		length = 0;
		return *this;
	}

	//------------------------------------------------
	// Specific methods.

	template <class key_type, class mapped_type, class hasher>
	hash_map<key_type, mapped_type, hasher>& hash_map<key_type, mapped_type, hasher>::insert(key_type key, mapped_type mapped)
	{
		node_type* existing = find(key);
		if (existing != nullptr)
		{
			existing->get().second = std::move(mapped);
			return *this;
		}

		grow_if_needed();
		const size_type bucket = bucket_of(key);
		node_type* fresh = new node_type(value_type(std::move(key), std::move(mapped)));
		fresh->next = buckets[bucket];
		buckets[bucket] = fresh;
		++length;
		return *this;
	}

	template <class key_type, class mapped_type, class hasher>
	hash_map<key_type, mapped_type, hasher>& hash_map<key_type, mapped_type, hasher>::remove(const key_type& key)
	{
		if (bucket_count == 0)
			return *this;

		for (node_type** link = &buckets[bucket_of(key)]; *link != nullptr; link = &(*link)->next)
		{
			if (!((*link)->get().first == key))
				continue;

			delete std::exchange(*link, (*link)->next);
			--length;
			break;
		}

		return *this;
	}

	//------------------------------------------------
	// Constant methods.

	template <class key_type, class mapped_type, class hasher>
	bool hash_map<key_type, mapped_type, hasher>::operator==(const hash_map& other) const
	{
		if (length != other.length)
			return false;

		for (const value_type& entry : *this)
		{
			const node_type* match = other.find(entry.first);
			if (match == nullptr || !(match->get().second == entry.second))
				return false;
		}

		return true;
	}

	template <class key_type, class mapped_type, class hasher>
	bool hash_map<key_type, mapped_type, hasher>::operator!=(const hash_map& other) const
	{
		return !(*this == other);
	}

	template <class key_type, class mapped_type, class hasher>
	typename hash_map<key_type, mapped_type, hasher>::size_type hash_map<key_type, mapped_type, hasher>::size() const noexcept
	{
		return length;
	}

	template <class key_type, class mapped_type, class hasher>
	bool hash_map<key_type, mapped_type, hasher>::empty() const noexcept
	{
		return length == 0;
	}

	template <class key_type, class mapped_type, class hasher>
	void hash_map<key_type, mapped_type, hasher>::print() const
	{
		std::cout << *this << '\n';
	}

	template <class key_type, class mapped_type, class hasher>
	mapped_type& hash_map<key_type, mapped_type, hasher>::get(const key_type& key)
	{
		node_type* match = find(key);
		if (match == nullptr)
			throw key_error("hash_map::get");
		return match->get().second;
	}

	template <class key_type, class mapped_type, class hasher>
	const mapped_type& hash_map<key_type, mapped_type, hasher>::get(const key_type& key) const
	{
		const node_type* match = find(key);
		if (match == nullptr)
			throw key_error("hash_map::get");
		return match->get().second;
	}

	template <class key_type, class mapped_type, class hasher>
	bool hash_map<key_type, mapped_type, hasher>::contains(const key_type& key) const
	{
		return find(key) != nullptr;
	}

	//------------------------------------------------
	// Iterator methods.

	template <class key_type, class mapped_type, class hasher>
	typename hash_map<key_type, mapped_type, hasher>::iterator hash_map<key_type, mapped_type, hasher>::begin() noexcept
	{
		return iterator(buckets, bucket_count, 0);
	}

	template <class key_type, class mapped_type, class hasher>
	typename hash_map<key_type, mapped_type, hasher>::iterator hash_map<key_type, mapped_type, hasher>::end() noexcept
	{
		return iterator(buckets, bucket_count, bucket_count);
	}

	template <class key_type, class mapped_type, class hasher>
	typename hash_map<key_type, mapped_type, hasher>::const_iterator hash_map<key_type, mapped_type, hasher>::begin() const noexcept
	{
		return const_iterator(buckets, bucket_count, 0);
	}

	template <class key_type, class mapped_type, class hasher>
	typename hash_map<key_type, mapped_type, hasher>::const_iterator hash_map<key_type, mapped_type, hasher>::end() const noexcept
	{
		return const_iterator(buckets, bucket_count, bucket_count);
	}

	//------------------------------------------------
	// Private helpers.

	template <class key_type, class mapped_type, class hasher>
	typename hash_map<key_type, mapped_type, hasher>::size_type hash_map<key_type, mapped_type, hasher>::bucket_of(const key_type& key) const
	{
		return hashing(key) & (bucket_count - 1);
	}

	template <class key_type, class mapped_type, class hasher>
	typename hash_map<key_type, mapped_type, hasher>::node_type* hash_map<key_type, mapped_type, hasher>::find(const key_type& key) const
	{
		if (bucket_count == 0)
			return nullptr;

		for (node_type* current = buckets[bucket_of(key)]; current != nullptr; current = current->next)
			if (current->get().first == key)
				return current;
		return nullptr;
	}

	template <class key_type, class mapped_type, class hasher>
	void hash_map<key_type, mapped_type, hasher>::grow_if_needed()
	{
		if (bucket_count == 0)
			rehash(detail::default_bucket_count);
		else if (length + 1 > bucket_count * detail::maximum_load_factor)
			rehash(bucket_count * detail::growth_factor);
	}

	// Moves every node into a fresh bucket array; nodes are relinked, never copied.
	template <class key_type, class mapped_type, class hasher>
	void hash_map<key_type, mapped_type, hasher>::rehash(size_type replacement_count)
	{
		node_type** fresh = new node_type*[replacement_count]();
		const size_type mask = replacement_count - 1;
		for (size_type bucket = 0; bucket < bucket_count; ++bucket)
		{
			node_type* current = buckets[bucket];
			while (current != nullptr)
			{
				node_type* following = current->next;
				const size_type target = hashing(current->get().first) & mask;
				current->next = fresh[target];
				fresh[target] = current;
				current = following;
			}
		}

		delete[] buckets;
		buckets = fresh;
		bucket_count = replacement_count;
	}

	// Copies each chain into the same bucket of a table of the same size, keeping the chain order.
	template <class key_type, class mapped_type, class hasher>
	void hash_map<key_type, mapped_type, hasher>::copy_chains(const hash_map& other)
	{
		if (other.bucket_count == 0)
			return;

		buckets = new node_type*[other.bucket_count]();
		bucket_count = other.bucket_count;
		for (size_type bucket = 0; bucket < bucket_count; ++bucket)
		{
			node_type** link = &buckets[bucket];
			for (const node_type* source = other.buckets[bucket]; source != nullptr; source = source->next)
			{
				*link = new node_type(source->get());
				link = &(*link)->next;
				++length;
			}
		}
	}

	template <class key_type, class mapped_type, class hasher>
	void hash_map<key_type, mapped_type, hasher>::release() noexcept
	{
		clear();
		delete[] buckets;
		buckets = nullptr;
		bucket_count = 0;
	}

	template <class key_type, class mapped_type, class hasher>
	void hash_map<key_type, mapped_type, hasher>::steal(hash_map& other) noexcept
	{
		buckets = std::exchange(other.buckets, nullptr);
		bucket_count = std::exchange(other.bucket_count, 0);
		length = std::exchange(other.length, 0);
	}

	template <class key_type, class mapped_type, class hasher>
	void hash_map<key_type, mapped_type, hasher>::swap(hash_map& other) noexcept
	{
		using std::swap;
		swap(buckets, other.buckets);
		swap(bucket_count, other.bucket_count);
		swap(length, other.length);
		swap(hashing, other.hashing);
	}
}
