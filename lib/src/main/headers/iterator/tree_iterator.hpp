#pragma once
#include "node/tree_node.hpp"
#include "utils/exceptions.hpp"

namespace ds
{
	template <class balancing, class T, class compare>
	class tree;
}

namespace ds::detail
{
	// Walks a binary search tree in order through parent links, so it needs no stack.
	template <class node_type, class T>
	class tree_iterator
	{
	public:
		explicit tree_iterator(node_type* current) noexcept;

		T& operator*() const noexcept;
		tree_iterator& operator++() noexcept;
		bool operator!=(const tree_iterator& other) const noexcept;

	private:
		template <class, class, class>
		friend class ds::tree;

		node_type* current;
	};

	//------------------------------------------------

	template <class node_type, class T>
	tree_iterator<node_type, T>::tree_iterator(node_type* current) noexcept : current(current) {}

	template <class node_type, class T>
	T& tree_iterator<node_type, T>::operator*() const noexcept
	{
		if (current == nullptr)
			fail_fast("tree_iterator::operator*: dereferenced past the end");
		return current->get();
	}

	template <class node_type, class T>
	tree_iterator<node_type, T>& tree_iterator<node_type, T>::operator++() noexcept
	{
		if (current == nullptr)
			fail_fast("tree_iterator::operator++: advanced past the end");
		current = neighbor(current, side::right);
		return *this;
	}

	template <class node_type, class T>
	bool tree_iterator<node_type, T>::operator!=(const tree_iterator& other) const noexcept
	{
		return current != other.current;
	}
}
