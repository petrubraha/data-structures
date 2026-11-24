#pragma once
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include "detail/iterator/list_iterator.hpp"
#include "detail/node/list_node.hpp"
#include "detail/utils/conversion.hpp"
#include "detail/utils/exceptions.hpp"
#include "detail/utils/linear_queries.hpp"
#include "detail/utils/printing.hpp"

namespace ds
{
	// Singly linked list with a tail pointer: O(1) at both ends for insertion, at the front for removal.
	template <class T, class compare = std::less<T>>
	class linked_list
	{
		using node_type = detail::list_node<T>;

	public:
		using value_type = T;
		using size_type = std::size_t;
		using reference = T&;
		using const_reference = const T&;
		using comparator = compare;
		using iterator = detail::list_iterator<node_type, T>;
		using const_iterator = detail::list_iterator<node_type, const T>;

		// Constructors and destructor.
		~linked_list();
		linked_list() noexcept(std::is_nothrow_default_constructible_v<comparator>);
		linked_list(std::initializer_list<T> initial);
		linked_list(const T* source, size_type count);
		linked_list(const linked_list& other);
		linked_list(linked_list&& other) noexcept(std::is_nothrow_move_constructible_v<comparator>);

		// Modifier methods.
		linked_list& operator=(const linked_list& other);
		linked_list& operator=(linked_list&& other) noexcept(std::is_nothrow_move_assignable_v<comparator>);
		linked_list& clear() noexcept;
		linked_list& set_compare(comparator replacement);

		// Specific methods.
		linked_list& insert(T value);
		linked_list& insert(T value, size_type index);
		linked_list& insert_after(const_iterator position, T value);
		linked_list& remove(size_type index);
		linked_list& remove_after(const_iterator position);

		// Constant methods.
		bool operator==(const linked_list& other) const;
		bool operator!=(const linked_list& other) const;
		size_type size() const noexcept;
		bool empty() const noexcept;
		void print() const;
		const_reference front() const;
		const_reference back() const;

		// Query operations.
		const_iterator search(const_reference value) const;
		const_iterator minimum() const;
		const_iterator maximum() const;
		const_iterator predecessor(const_iterator position) const;
		const_iterator successor(const_iterator position) const;

		// Iterator methods.
		iterator begin() noexcept;
		iterator end() noexcept;
		const_iterator begin() const noexcept;
		const_iterator end() const noexcept;

		// Friend functions.
		friend std::ostream& operator<<(std::ostream& out, const linked_list& list)
		{
			return detail::write_elements(out, list);
		}

		friend std::unique_ptr<T[]> convert(const linked_list& list)
		{
			return detail::copy_elements<T>(list, list.length);
		}

	private:
		node_type* head = nullptr;
		node_type* tail = nullptr;
		size_type length = 0;
		comparator order{};

		template <class position>
		void append_all(detail::position_range<position> source);
		void link_front(node_type* fresh) noexcept;
		void link_after(node_type* previous, node_type* fresh) noexcept;
		void unlink_front() noexcept;
		void unlink_after(node_type* previous) noexcept;
		node_type* node_at(size_type index) const noexcept;
		void steal(linked_list& other) noexcept;
		void swap(linked_list& other) noexcept;
		detail::position_range<const_iterator> stored() const noexcept;
	};

	//------------------------------------------------
	// Constructors and destructor.

	template <class T, class compare>
	linked_list<T, compare>::~linked_list()
	{
		clear();
	}

	template <class T, class compare>
	linked_list<T, compare>::linked_list() noexcept(std::is_nothrow_default_constructible_v<comparator>) {}

	template <class T, class compare>
	linked_list<T, compare>::linked_list(std::initializer_list<T> initial) : linked_list(initial.begin(), initial.size()) {}

	template <class T, class compare>
	linked_list<T, compare>::linked_list(const T* source, size_type count)
	{
		append_all(detail::position_range<const T*>{ source, source + count });
	}

	template <class T, class compare>
	linked_list<T, compare>::linked_list(const linked_list& other) : order(other.order)
	{
		append_all(other.stored());
	}

	template <class T, class compare>
	linked_list<T, compare>::linked_list(linked_list&& other) noexcept(std::is_nothrow_move_constructible_v<comparator>)
		: order(std::move(other.order))
	{
		steal(other);
	}

	//------------------------------------------------
	// Modifier methods.

	template <class T, class compare>
	linked_list<T, compare>& linked_list<T, compare>::operator=(const linked_list& other)
	{
		linked_list copy(other);
		swap(copy);
		return *this;
	}

	template <class T, class compare>
	linked_list<T, compare>& linked_list<T, compare>::operator=(linked_list&& other) noexcept(std::is_nothrow_move_assignable_v<comparator>)
	{
		if (this == &other)
			return *this;

		clear();
		steal(other);
		order = std::move(other.order);
		return *this;
	}

	template <class T, class compare>
	linked_list<T, compare>& linked_list<T, compare>::clear() noexcept
	{
		while (head != nullptr)
			unlink_front();
		return *this;
	}

	template <class T, class compare>
	linked_list<T, compare>& linked_list<T, compare>::set_compare(comparator replacement)
	{
		order = std::move(replacement);
		return *this;
	}

	//------------------------------------------------
	// Specific methods.

	template <class T, class compare>
	linked_list<T, compare>& linked_list<T, compare>::insert(T value)
	{
		return insert(std::move(value), length);
	}

	template <class T, class compare>
	linked_list<T, compare>& linked_list<T, compare>::insert(T value, size_type index)
	{
		if (index > length)
			throw index_error("linked_list::insert", index, length);

		node_type* fresh = new node_type(std::move(value));
		if (index == 0)
			link_front(fresh);
		else
			link_after(index == length ? tail : node_at(index - 1), fresh);
		return *this;
	}

	template <class T, class compare>
	linked_list<T, compare>& linked_list<T, compare>::insert_after(const_iterator position, T value)
	{
		if (position.current == nullptr)
			throw position_error("linked_list::insert_after", "position is the end");

		link_after(position.current, new node_type(std::move(value)));
		return *this;
	}

	template <class T, class compare>
	linked_list<T, compare>& linked_list<T, compare>::remove(size_type index)
	{
		if (index >= length)
			throw index_error("linked_list::remove", index, length);

		if (index == 0)
			unlink_front();
		else
			unlink_after(node_at(index - 1));
		return *this;
	}

	template <class T, class compare>
	linked_list<T, compare>& linked_list<T, compare>::remove_after(const_iterator position)
	{
		if (position.current == nullptr || position.current->next == nullptr)
			throw position_error("linked_list::remove_after", "no element after the position");

		unlink_after(position.current);
		return *this;
	}

	//------------------------------------------------
	// Constant methods.

	template <class T, class compare>
	bool linked_list<T, compare>::operator==(const linked_list& other) const
	{
		if (length != other.length)
			return false;

		for (const node_type *mine = head, *theirs = other.head; mine != nullptr; mine = mine->next, theirs = theirs->next)
			if (!(mine->get() == theirs->get()))
				return false;
		return true;
	}

	template <class T, class compare>
	bool linked_list<T, compare>::operator!=(const linked_list& other) const
	{
		return !(*this == other);
	}

	template <class T, class compare>
	typename linked_list<T, compare>::size_type linked_list<T, compare>::size() const noexcept
	{
		return length;
	}

	template <class T, class compare>
	bool linked_list<T, compare>::empty() const noexcept
	{
		return length == 0;
	}

	template <class T, class compare>
	void linked_list<T, compare>::print() const
	{
		std::cout << *this << '\n';
	}

	template <class T, class compare>
	typename linked_list<T, compare>::const_reference linked_list<T, compare>::front() const
	{
		if (empty())
			throw empty_error("linked_list::front");
		return head->get();
	}

	template <class T, class compare>
	typename linked_list<T, compare>::const_reference linked_list<T, compare>::back() const
	{
		if (empty())
			throw empty_error("linked_list::back");
		return tail->get();
	}

	//------------------------------------------------
	// Query operations.

	template <class T, class compare>
	typename linked_list<T, compare>::const_iterator linked_list<T, compare>::search(const_reference value) const
	{
		return detail::find(stored(), value);
	}

	template <class T, class compare>
	typename linked_list<T, compare>::const_iterator linked_list<T, compare>::minimum() const
	{
		if (empty())
			throw empty_error("linked_list::minimum");
		return detail::first_extreme(stored(), order);
	}

	template <class T, class compare>
	typename linked_list<T, compare>::const_iterator linked_list<T, compare>::maximum() const
	{
		if (empty())
			throw empty_error("linked_list::maximum");
		return detail::first_extreme(stored(), detail::reversed<comparator>(order));
	}

	template <class T, class compare>
	typename linked_list<T, compare>::const_iterator linked_list<T, compare>::predecessor(const_iterator position) const
	{
		return detail::closest_before(stored(), *position, order);
	}

	template <class T, class compare>
	typename linked_list<T, compare>::const_iterator linked_list<T, compare>::successor(const_iterator position) const
	{
		return detail::closest_before(stored(), *position, detail::reversed<comparator>(order));
	}

	//------------------------------------------------
	// Iterator methods.

	template <class T, class compare>
	typename linked_list<T, compare>::iterator linked_list<T, compare>::begin() noexcept
	{
		return iterator(head);
	}

	template <class T, class compare>
	typename linked_list<T, compare>::iterator linked_list<T, compare>::end() noexcept
	{
		return iterator(nullptr);
	}

	template <class T, class compare>
	typename linked_list<T, compare>::const_iterator linked_list<T, compare>::begin() const noexcept
	{
		return const_iterator(head);
	}

	template <class T, class compare>
	typename linked_list<T, compare>::const_iterator linked_list<T, compare>::end() const noexcept
	{
		return const_iterator(nullptr);
	}

	//------------------------------------------------
	// Private helpers.

	// Appends copies of the source; on failure the partially built chain is released before rethrowing.
	template <class T, class compare>
	template <class position>
	void linked_list<T, compare>::append_all(detail::position_range<position> source)
	{
		try
		{
			for (position current = source.first; current != source.last; ++current)
				link_after(tail, new node_type(*current));
		}
		catch (...)
		{
			clear();
			throw;
		}
	}

	template <class T, class compare>
	void linked_list<T, compare>::link_front(node_type* fresh) noexcept
	{
		fresh->next = head;
		head = fresh;
		if (tail == nullptr)
			tail = fresh;
		++length;
	}

	// A null previous node means the list is empty, so the fresh node becomes the front.
	template <class T, class compare>
	void linked_list<T, compare>::link_after(node_type* previous, node_type* fresh) noexcept
	{
		if (previous == nullptr)
		{
			link_front(fresh);
			return;
		}

		fresh->next = previous->next;
		previous->next = fresh;
		if (previous == tail)
			tail = fresh;
		++length;
	}

	template <class T, class compare>
	void linked_list<T, compare>::unlink_front() noexcept
	{
		node_type* removed = head;
		head = head->next;
		if (head == nullptr)
			tail = nullptr;

		delete removed;
		--length;
	}

	template <class T, class compare>
	void linked_list<T, compare>::unlink_after(node_type* previous) noexcept
	{
		node_type* removed = previous->next;
		previous->next = removed->next;
		if (removed == tail)
			tail = previous;

		delete removed;
		--length;
	}

	template <class T, class compare>
	typename linked_list<T, compare>::node_type* linked_list<T, compare>::node_at(size_type index) const noexcept
	{
		node_type* current = head;
		for (size_type step = 0; step < index; ++step)
			current = current->next;
		return current;
	}

	template <class T, class compare>
	void linked_list<T, compare>::steal(linked_list& other) noexcept
	{
		head = std::exchange(other.head, nullptr);
		tail = std::exchange(other.tail, nullptr);
		length = std::exchange(other.length, 0);
	}

	template <class T, class compare>
	void linked_list<T, compare>::swap(linked_list& other) noexcept
	{
		using std::swap;
		swap(head, other.head);
		swap(tail, other.tail);
		swap(length, other.length);
		swap(order, other.order);
	}

	template <class T, class compare>
	detail::position_range<typename linked_list<T, compare>::const_iterator> linked_list<T, compare>::stored() const noexcept
	{
		return { begin(), end() };
	}
}
