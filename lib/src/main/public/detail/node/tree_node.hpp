#pragma once
#include <cstddef>
#include "node.hpp"

namespace ds::detail
{
	enum class side : unsigned char
	{
		left,
		right
	};

	constexpr side opposite(side direction) noexcept
	{
		return direction == side::left ? side::right : side::left;
	}

	// Binary search tree node. The balancing policy decides what the augmentation stores (height, colour).
	template <class T, class augmentation>
	class tree_node : public node<T>
	{
	public:
		explicit tree_node(T initial) : node<T>(std::move(initial)) {}

		tree_node*& child(side direction) noexcept;
		tree_node* child(side direction) const noexcept;

		tree_node* parent = nullptr;
		augmentation augment{};

	private:
		tree_node* children[2] = { nullptr, nullptr };
	};

	// The side of its parent that a non-root node hangs on.
	template <class node_type>
	side side_in_parent(const node_type* child) noexcept;

	// The furthest node reachable from the subtree root by following one side.
	template <class node_type>
	node_type* extreme(node_type* subtree, side direction) noexcept;

	// The in-order neighbour: the successor on the right side, the predecessor on the left side.
	template <class node_type>
	node_type* neighbor(node_type* current, side direction) noexcept;

	// Puts the replacement (which may be null) where the replaced node hangs, updating the root if needed.
	template <class node_type>
	void transplant(node_type*& root, node_type* replaced, node_type* replacement) noexcept;

	// Moves the pivot down towards the direction; its child on the opposite side takes its place.
	template <class node_type>
	void rotate(node_type*& root, node_type* pivot, side direction) noexcept;

	//------------------------------------------------

	template <class T, class augmentation>
	tree_node<T, augmentation>*& tree_node<T, augmentation>::child(side direction) noexcept
	{
		return children[static_cast<std::size_t>(direction)];
	}

	template <class T, class augmentation>
	tree_node<T, augmentation>* tree_node<T, augmentation>::child(side direction) const noexcept
	{
		return children[static_cast<std::size_t>(direction)];
	}

	template <class node_type>
	side side_in_parent(const node_type* child) noexcept
	{
		return child == child->parent->child(side::left) ? side::left : side::right;
	}

	template <class node_type>
	node_type* extreme(node_type* subtree, side direction) noexcept
	{
		while (subtree->child(direction) != nullptr)
			subtree = subtree->child(direction);
		return subtree;
	}

	template <class node_type>
	node_type* neighbor(node_type* current, side direction) noexcept
	{
		node_type* below = current->child(direction);
		if (below != nullptr)
			return extreme(below, opposite(direction));

		node_type* ancestor = current->parent;
		while (ancestor != nullptr && current == ancestor->child(direction))
		{
			current = ancestor;
			ancestor = ancestor->parent;
		}

		return ancestor;
	}

	template <class node_type>
	void transplant(node_type*& root, node_type* replaced, node_type* replacement) noexcept
	{
		node_type* parent = replaced->parent;
		if (parent == nullptr)
			root = replacement;
		else
			parent->child(side_in_parent(replaced)) = replacement;

		if (replacement != nullptr)
			replacement->parent = parent;
	}

	template <class node_type>
	void rotate(node_type*& root, node_type* pivot, side direction) noexcept
	{
		const side rising_side = opposite(direction);
		node_type* riser = pivot->child(rising_side);
		node_type* moved = riser->child(direction);

		pivot->child(rising_side) = moved;
		if (moved != nullptr)
			moved->parent = pivot;

		transplant(root, pivot, riser);
		riser->child(direction) = pivot;
		pivot->parent = riser;
	}
}
