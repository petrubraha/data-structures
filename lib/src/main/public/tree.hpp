#pragma once
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include "iterator/tree_iterator.hpp"
#include "node/tree_node.hpp"
#include "policy/balancing.hpp"
#include "utils/conversion.hpp"
#include "utils/exceptions.hpp"
#include "utils/printing.hpp"

namespace ds::detail
{
	// Test hook: declared here, defined only by the test suite to inspect the node structure.
	struct tree_inspector;
}

namespace ds
{
	// Self-balancing binary search tree of unique values. The balancing tag (avl or red_black) picks the node
	// augmentation and the rebalance step; search, traversal and queries are shared.
	template <class balancing, class T, class compare = std::less<T>>
	class tree
	{
		using node_type = detail::tree_node<T, typename balancing::augmentation>;

		// Where a value belongs: the existing node holding it, or the parent and side to attach it under.
		struct slot
		{
			node_type* existing;
			node_type* parent;
			detail::side direction;
		};

	public:
		using value_type = T;
		using size_type = std::size_t;
		using const_reference = const T&;
		using comparator = compare;
		using const_iterator = detail::tree_iterator<node_type, const T>;

		// Constructors and destructor. Duplicate source values are kept once.
		~tree();
		tree() noexcept(std::is_nothrow_default_constructible_v<comparator>);
		tree(std::initializer_list<T> initial);
		tree(const T* source, size_type count);
		tree(const tree& other);
		tree(tree&& other) noexcept(std::is_nothrow_move_constructible_v<comparator>);

		// Modifier methods.
		tree& operator=(const tree& other);
		tree& operator=(tree&& other) noexcept(std::is_nothrow_move_assignable_v<comparator>);
		tree& clear() noexcept;
		tree& set_compare(comparator replacement);

		// Specific methods. Inserting a value already present and removing a missing one change nothing.
		tree& insert(T value);
		tree& remove(const_reference value);

		// Constant methods.
		bool operator==(const tree& other) const;
		bool operator!=(const tree& other) const;
		size_type size() const noexcept;
		bool empty() const noexcept;
		void print() const;

		// Query operations.
		const_iterator search(const_reference value) const;
		const_iterator minimum() const;
		const_iterator maximum() const;
		const_iterator predecessor(const_iterator position) const;
		const_iterator successor(const_iterator position) const;

		// Iterator methods. Traversal is in order; values are read-only to keep the tree ordered.
		const_iterator begin() const noexcept;
		const_iterator end() const noexcept;

		// Friend functions.
		friend std::ostream& operator<<(std::ostream& out, const tree& values)
		{
			return detail::write_elements(out, values);
		}

		friend std::unique_ptr<T[]> convert(const tree& values)
		{
			return detail::copy_elements<T>(values, values.length);
		}

		friend struct detail::tree_inspector;

	private:
		node_type* root = nullptr;
		size_type length = 0;
		comparator order{};

		slot find_slot(const T& value) const;
		void attach(node_type* fresh, const slot& place) noexcept;
		void unlink(node_type* removed) noexcept;
		void copy_subtree(const node_type* source, node_type* parent, detail::side direction);
		node_type* detach_all() noexcept;
		const_iterator neighbor_of(const char* operation, const_iterator position, detail::side direction) const;
		void steal(tree& other) noexcept;
		void swap(tree& other) noexcept;
	};

	//------------------------------------------------
	// Constructors and destructor.

	template <class balancing, class T, class compare>
	tree<balancing, T, compare>::~tree()
	{
		clear();
	}

	template <class balancing, class T, class compare>
	tree<balancing, T, compare>::tree() noexcept(std::is_nothrow_default_constructible_v<comparator>) {}

	template <class balancing, class T, class compare>
	tree<balancing, T, compare>::tree(std::initializer_list<T> initial) : tree(initial.begin(), initial.size()) {}

	template <class balancing, class T, class compare>
	tree<balancing, T, compare>::tree(const T* source, size_type count) : tree()
	{
		for (size_type index = 0; index < count; ++index)
			insert(source[index]);
	}

	template <class balancing, class T, class compare>
	tree<balancing, T, compare>::tree(const tree& other) : order(other.order)
	{
		try
		{
			if (other.root != nullptr)
				copy_subtree(other.root, nullptr, detail::side::left);
		}
		catch (...)
		{
			clear();
			throw;
		}
	}

	template <class balancing, class T, class compare>
	tree<balancing, T, compare>::tree(tree&& other) noexcept(std::is_nothrow_move_constructible_v<comparator>)
		: order(std::move(other.order))
	{
		steal(other);
	}

	//------------------------------------------------
	// Modifier methods.

	template <class balancing, class T, class compare>
	tree<balancing, T, compare>& tree<balancing, T, compare>::operator=(const tree& other)
	{
		tree copy(other);
		swap(copy);
		return *this;
	}

	template <class balancing, class T, class compare>
	tree<balancing, T, compare>& tree<balancing, T, compare>::operator=(tree&& other) noexcept(std::is_nothrow_move_assignable_v<comparator>)
	{
		if (this == &other)
			return *this;

		clear();
		steal(other);
		order = std::move(other.order);
		return *this;
	}

	template <class balancing, class T, class compare>
	tree<balancing, T, compare>& tree<balancing, T, compare>::clear() noexcept
	{
		node_type* pending = detach_all();
		while (pending != nullptr)
			delete std::exchange(pending, pending->parent);
		return *this;
	}

	// Relinks every existing node under the new order; values that become equivalent are dropped.
	template <class balancing, class T, class compare>
	tree<balancing, T, compare>& tree<balancing, T, compare>::set_compare(comparator replacement)
	{
		order = std::move(replacement);
		node_type* pending = detach_all();
		while (pending != nullptr)
		{
			node_type* relinked = std::exchange(pending, pending->parent);
			relinked->parent = nullptr;
			relinked->augment = {};

			const slot place = find_slot(relinked->get());
			if (place.existing != nullptr)
				delete relinked;
			else
				attach(relinked, place);
		}

		return *this;
	}

	//------------------------------------------------
	// Specific methods.

	template <class balancing, class T, class compare>
	tree<balancing, T, compare>& tree<balancing, T, compare>::insert(T value)
	{
		const slot place = find_slot(value);
		if (place.existing == nullptr)
			attach(new node_type(std::move(value)), place);
		return *this;
	}

	template <class balancing, class T, class compare>
	tree<balancing, T, compare>& tree<balancing, T, compare>::remove(const_reference value)
	{
		node_type* found = find_slot(value).existing;
		if (found != nullptr)
			unlink(found);
		return *this;
	}

	//------------------------------------------------
	// Constant methods.

	template <class balancing, class T, class compare>
	bool tree<balancing, T, compare>::operator==(const tree& other) const
	{
		if (length != other.length)
			return false;

		const_iterator theirs = other.begin();
		for (const T& mine : *this)
		{
			if (!(mine == *theirs))
				return false;
			++theirs;
		}

		return true;
	}

	template <class balancing, class T, class compare>
	bool tree<balancing, T, compare>::operator!=(const tree& other) const
	{
		return !(*this == other);
	}

	template <class balancing, class T, class compare>
	typename tree<balancing, T, compare>::size_type tree<balancing, T, compare>::size() const noexcept
	{
		return length;
	}

	template <class balancing, class T, class compare>
	bool tree<balancing, T, compare>::empty() const noexcept
	{
		return length == 0;
	}

	template <class balancing, class T, class compare>
	void tree<balancing, T, compare>::print() const
	{
		std::cout << *this << '\n';
	}

	//------------------------------------------------
	// Query operations.

	template <class balancing, class T, class compare>
	typename tree<balancing, T, compare>::const_iterator tree<balancing, T, compare>::search(const_reference value) const
	{
		return const_iterator(find_slot(value).existing);
	}

	template <class balancing, class T, class compare>
	typename tree<balancing, T, compare>::const_iterator tree<balancing, T, compare>::minimum() const
	{
		if (empty())
			throw empty_error("tree::minimum");
		return const_iterator(detail::extreme(root, detail::side::left));
	}

	template <class balancing, class T, class compare>
	typename tree<balancing, T, compare>::const_iterator tree<balancing, T, compare>::maximum() const
	{
		if (empty())
			throw empty_error("tree::maximum");
		return const_iterator(detail::extreme(root, detail::side::right));
	}

	template <class balancing, class T, class compare>
	typename tree<balancing, T, compare>::const_iterator tree<balancing, T, compare>::predecessor(const_iterator position) const
	{
		return neighbor_of("tree::predecessor", position, detail::side::left);
	}

	template <class balancing, class T, class compare>
	typename tree<balancing, T, compare>::const_iterator tree<balancing, T, compare>::successor(const_iterator position) const
	{
		return neighbor_of("tree::successor", position, detail::side::right);
	}

	//------------------------------------------------
	// Iterator methods.

	template <class balancing, class T, class compare>
	typename tree<balancing, T, compare>::const_iterator tree<balancing, T, compare>::begin() const noexcept
	{
		return const_iterator(root == nullptr ? nullptr : detail::extreme(root, detail::side::left));
	}

	template <class balancing, class T, class compare>
	typename tree<balancing, T, compare>::const_iterator tree<balancing, T, compare>::end() const noexcept
	{
		return const_iterator(nullptr);
	}

	//------------------------------------------------
	// Private helpers.

	template <class balancing, class T, class compare>
	typename tree<balancing, T, compare>::slot tree<balancing, T, compare>::find_slot(const T& value) const
	{
		slot place{ nullptr, nullptr, detail::side::left };
		for (node_type* current = root; current != nullptr; current = current->child(place.direction))
		{
			if (order(value, current->get()))
				place.direction = detail::side::left;
			else if (order(current->get(), value))
				place.direction = detail::side::right;
			else
			{
				place.existing = current;
				return place;
			}

			place.parent = current;
		}

		return place;
	}

	template <class balancing, class T, class compare>
	void tree<balancing, T, compare>::attach(node_type* fresh, const slot& place) noexcept
	{
		fresh->parent = place.parent;
		if (place.parent == nullptr)
			root = fresh;
		else
			place.parent->child(place.direction) = fresh;

		++length;
		balancing::after_insert(root, fresh);
	}

	// Unlinks the node, replacing it by its successor when it has two children, then lets the policy rebalance.
	template <class balancing, class T, class compare>
	void tree<balancing, T, compare>::unlink(node_type* removed) noexcept
	{
		using detail::side;
		detail::removal<node_type, typename balancing::augmentation> unlinked{ nullptr, removed->parent, removed->augment };

		if (removed->child(side::left) == nullptr || removed->child(side::right) == nullptr)
		{
			const side only = removed->child(side::left) == nullptr ? side::right : side::left;
			unlinked.child = removed->child(only);
			detail::transplant(root, removed, unlinked.child);
		}
		else
		{
			node_type* successor = detail::extreme(removed->child(side::right), side::left);
			unlinked.removed = successor->augment;
			unlinked.child = successor->child(side::right);
			unlinked.parent = successor;
			if (successor->parent != removed)
			{
				unlinked.parent = successor->parent;
				detail::transplant(root, successor, unlinked.child);
				successor->child(side::right) = removed->child(side::right);
				successor->child(side::right)->parent = successor;
			}

			detail::transplant(root, removed, successor);
			successor->child(side::left) = removed->child(side::left);
			successor->child(side::left)->parent = successor;
			successor->augment = removed->augment;
		}

		delete removed;
		--length;
		balancing::after_remove(root, unlinked);
	}

	template <class balancing, class T, class compare>
	void tree<balancing, T, compare>::copy_subtree(const node_type* source, node_type* parent, detail::side direction)
	{
		node_type* copy = new node_type(source->get());
		copy->augment = source->augment;
		copy->parent = parent;
		if (parent == nullptr)
			root = copy;
		else
			parent->child(direction) = copy;
		++length;

		for (const detail::side child_side : { detail::side::left, detail::side::right })
			if (source->child(child_side) != nullptr)
				copy_subtree(source->child(child_side), copy, child_side);
	}

	// Takes every node out of the tree in post-order, without recursion, and returns them as a chain
	// linked through the parent pointers. The tree is left empty.
	template <class balancing, class T, class compare>
	typename tree<balancing, T, compare>::node_type* tree<balancing, T, compare>::detach_all() noexcept
	{
		using detail::side;
		node_type* chain = nullptr;
		node_type* current = root;
		while (current != nullptr)
		{
			node_type* below = current->child(side::left) != nullptr ? current->child(side::left) : current->child(side::right);
			if (below != nullptr)
			{
				current = below;
				continue;
			}

			node_type* parent = current->parent;
			if (parent != nullptr)
				parent->child(detail::side_in_parent(current)) = nullptr;

			current->parent = chain;
			chain = current;
			current = parent;
		}

		root = nullptr;
		length = 0;
		return chain;
	}

	template <class balancing, class T, class compare>
	typename tree<balancing, T, compare>::const_iterator tree<balancing, T, compare>::neighbor_of(
		const char* operation, const_iterator position, detail::side direction) const
	{
		if (position.current == nullptr)
			throw position_error(operation, "position is the end");
		return const_iterator(detail::neighbor(position.current, direction));
	}

	template <class balancing, class T, class compare>
	void tree<balancing, T, compare>::steal(tree& other) noexcept
	{
		root = std::exchange(other.root, nullptr);
		length = std::exchange(other.length, 0);
	}

	template <class balancing, class T, class compare>
	void tree<balancing, T, compare>::swap(tree& other) noexcept
	{
		using std::swap;
		swap(root, other.root);
		swap(length, other.length);
		swap(order, other.order);
	}
}
