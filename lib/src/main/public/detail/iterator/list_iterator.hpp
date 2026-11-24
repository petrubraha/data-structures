#pragma once
#include <type_traits>
#include "detail/utils/exceptions.hpp"

namespace ds
{
	template <class T, class compare>
	class linked_list;
}

namespace ds::detail
{
	// Walks the next links of a list or deque node chain. A const-qualified T gives read-only traversal.
	template <class node_type, class T>
	class list_iterator
	{
	public:
		explicit list_iterator(node_type* current) noexcept;

		// Lets a mutable iterator convert to its read-only counterpart.
		template <class mutable_type, class = std::enable_if_t<std::is_same_v<const mutable_type, T>>>
		list_iterator(const list_iterator<node_type, mutable_type>& other) noexcept : current(other.current) {}

		T& operator*() const noexcept;
		list_iterator& operator++() noexcept;
		bool operator!=(const list_iterator& other) const noexcept;

	private:
		template <class, class>
		friend class list_iterator;

		template <class, class>
		friend class ds::linked_list;

		node_type* current;
	};

	//------------------------------------------------

	template <class node_type, class T>
	list_iterator<node_type, T>::list_iterator(node_type* current) noexcept : current(current) {}

	template <class node_type, class T>
	T& list_iterator<node_type, T>::operator*() const noexcept
	{
		if (current == nullptr)
			fail_fast("list_iterator::operator*: dereferenced past the end");
		return current->get();
	}

	template <class node_type, class T>
	list_iterator<node_type, T>& list_iterator<node_type, T>::operator++() noexcept
	{
		if (current == nullptr)
			fail_fast("list_iterator::operator++: advanced past the end");
		current = current->next;
		return *this;
	}

	template <class node_type, class T>
	bool list_iterator<node_type, T>::operator!=(const list_iterator& other) const noexcept
	{
		return current != other.current;
	}
}
