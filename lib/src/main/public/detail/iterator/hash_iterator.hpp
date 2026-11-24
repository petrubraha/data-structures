#pragma once
#include <cstddef>
#include "detail/utils/exceptions.hpp"

namespace ds::detail
{
	// Walks every chain of a bucket array in bucket order. A const-qualified T gives read-only traversal.
	template <class node_type, class T>
	class hash_iterator
	{
	public:
		// Starts at the first node at or after the bucket; a bucket equal to the count is the end.
		hash_iterator(node_type* const* buckets, std::size_t bucket_count, std::size_t bucket) noexcept;

		T& operator*() const noexcept;
		hash_iterator& operator++() noexcept;
		bool operator!=(const hash_iterator& other) const noexcept;

	private:
		node_type* const* buckets;
		std::size_t bucket_count;
		std::size_t bucket;
		node_type* current = nullptr;

		void skip_empty_buckets() noexcept;
	};

	//------------------------------------------------

	template <class node_type, class T>
	hash_iterator<node_type, T>::hash_iterator(node_type* const* buckets, std::size_t bucket_count, std::size_t bucket) noexcept
		: buckets(buckets), bucket_count(bucket_count), bucket(bucket)
	{
		if (bucket < bucket_count)
			current = buckets[bucket];
		skip_empty_buckets();
	}

	template <class node_type, class T>
	T& hash_iterator<node_type, T>::operator*() const noexcept
	{
		if (current == nullptr)
			fail_fast("hash_iterator::operator*: dereferenced past the end");
		return current->get();
	}

	template <class node_type, class T>
	hash_iterator<node_type, T>& hash_iterator<node_type, T>::operator++() noexcept
	{
		if (current == nullptr)
			fail_fast("hash_iterator::operator++: advanced past the end");

		current = current->next;
		skip_empty_buckets();
		return *this;
	}

	template <class node_type, class T>
	bool hash_iterator<node_type, T>::operator!=(const hash_iterator& other) const noexcept
	{
		return current != other.current;
	}

	template <class node_type, class T>
	void hash_iterator<node_type, T>::skip_empty_buckets() noexcept
	{
		while (current == nullptr && bucket + 1 < bucket_count)
			current = buckets[++bucket];
	}
}
