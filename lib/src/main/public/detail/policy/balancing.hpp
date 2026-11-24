#pragma once
#include <algorithm>
#include "node/tree_node.hpp"

namespace ds::detail
{
	// What the tree core reports after unlinking a node: the child that took the removed position (maybe
	// null), that child's parent, and the augmentation of the node that physically left the tree.
	template <class node_type, class augmentation>
	struct removal
	{
		node_type* child;
		node_type* parent;
		augmentation removed;
	};
}

// Tree balancing tags. The binary search tree core links and unlinks nodes; the tag picks the node
// augmentation and restores balance afterwards.
namespace ds
{
	// Height-balanced: the subtrees of every node differ in height by at most one.
	struct avl
	{
		// Height of the subtree rooted at the node; a leaf has height one.
		using augmentation = unsigned char;

		template <class node_type>
		static void after_insert(node_type*& root, node_type* inserted) noexcept;

		template <class node_type>
		static void after_remove(node_type*& root, const detail::removal<node_type, augmentation>& unlinked) noexcept;

	private:
		template <class node_type>
		static augmentation height(const node_type* subtree) noexcept;

		template <class node_type>
		static void update_height(node_type* subtree) noexcept;

		template <class node_type>
		static void rotate_and_update(node_type*& root, node_type* pivot, detail::side direction) noexcept;

		template <class node_type>
		static node_type* rebalance(node_type*& root, node_type* subtree) noexcept;

		template <class node_type>
		static void rebalance_up(node_type*& root, node_type* start) noexcept;
	};

	// Colour-balanced: no red node has a red child, and every root-to-leaf path has the same black count.
	struct red_black
	{
		enum class colour : unsigned char
		{
			red,
			black
		};

		// New nodes start red, the value-initialized colour.
		using augmentation = colour;

		template <class node_type>
		static void after_insert(node_type*& root, node_type* inserted) noexcept;

		template <class node_type>
		static void after_remove(node_type*& root, const detail::removal<node_type, augmentation>& unlinked) noexcept;

	private:
		template <class node_type>
		static bool is_red(const node_type* subtree) noexcept;

		template <class node_type>
		static void paint(node_type* subtree, colour shade) noexcept;
	};

	//------------------------------------------------
	// avl

	template <class node_type>
	void avl::after_insert(node_type*& root, node_type* inserted) noexcept
	{
		inserted->augment = 1;
		rebalance_up(root, inserted->parent);
	}

	template <class node_type>
	void avl::after_remove(node_type*& root, const detail::removal<node_type, augmentation>& unlinked) noexcept
	{
		rebalance_up(root, unlinked.parent);
	}

	template <class node_type>
	avl::augmentation avl::height(const node_type* subtree) noexcept
	{
		return subtree == nullptr ? augmentation{ 0 } : subtree->augment;
	}

	template <class node_type>
	void avl::update_height(node_type* subtree) noexcept
	{
		const augmentation left = height(subtree->child(detail::side::left));
		const augmentation right = height(subtree->child(detail::side::right));
		subtree->augment = static_cast<augmentation>(1 + std::max(left, right));
	}

	template <class node_type>
	void avl::rotate_and_update(node_type*& root, node_type* pivot, detail::side direction) noexcept
	{
		detail::rotate(root, pivot, direction);
		update_height(pivot);
		update_height(pivot->parent);
	}

	// Returns the node that roots the subtree after any rotation.
	template <class node_type>
	node_type* avl::rebalance(node_type*& root, node_type* subtree) noexcept
	{
		const int left = height(subtree->child(detail::side::left));
		const int right = height(subtree->child(detail::side::right));
		const int imbalance = left - right;
		if (imbalance >= -1 && imbalance <= 1)
			return subtree;

		const detail::side heavy = imbalance > 1 ? detail::side::left : detail::side::right;
		node_type* child = subtree->child(heavy);
		if (height(child->child(detail::opposite(heavy))) > height(child->child(heavy)))
			rotate_and_update(root, child, heavy);

		rotate_and_update(root, subtree, detail::opposite(heavy));
		return subtree->parent;
	}

	template <class node_type>
	void avl::rebalance_up(node_type*& root, node_type* start) noexcept
	{
		for (node_type* current = start; current != nullptr;)
		{
			update_height(current);
			current = rebalance(root, current)->parent;
		}
	}

	//------------------------------------------------
	// red_black

	template <class node_type>
	void red_black::after_insert(node_type*& root, node_type* inserted) noexcept
	{
		node_type* current = inserted;
		while (is_red(current->parent))
		{
			node_type* parent = current->parent;
			node_type* grandparent = parent->parent;
			const detail::side parent_side = detail::side_in_parent(parent);
			node_type* uncle = grandparent->child(detail::opposite(parent_side));

			if (is_red(uncle))
			{
				paint(parent, colour::black);
				paint(uncle, colour::black);
				paint(grandparent, colour::red);
				current = grandparent;
				continue;
			}

			if (current == parent->child(detail::opposite(parent_side)))
			{
				current = parent;
				detail::rotate(root, current, parent_side);
				parent = current->parent;
			}

			paint(parent, colour::black);
			paint(grandparent, colour::red);
			detail::rotate(root, grandparent, detail::opposite(parent_side));
		}

		paint(root, colour::black);
	}

	// Removing a black node leaves its path one black short; the loop pushes that deficit up or resolves it.
	template <class node_type>
	void red_black::after_remove(node_type*& root, const detail::removal<node_type, augmentation>& unlinked) noexcept
	{
		if (unlinked.removed == colour::red)
			return;

		node_type* current = unlinked.child;
		node_type* parent = unlinked.parent;
		while (current != root && !is_red(current))
		{
			const detail::side current_side = current == parent->child(detail::side::left) ? detail::side::left : detail::side::right;
			const detail::side sibling_side = detail::opposite(current_side);
			node_type* sibling = parent->child(sibling_side);

			if (is_red(sibling))
			{
				paint(sibling, colour::black);
				paint(parent, colour::red);
				detail::rotate(root, parent, current_side);
				sibling = parent->child(sibling_side);
			}

			if (!is_red(sibling->child(detail::side::left)) && !is_red(sibling->child(detail::side::right)))
			{
				paint(sibling, colour::red);
				current = parent;
				parent = current->parent;
				continue;
			}

			if (!is_red(sibling->child(sibling_side)))
			{
				paint(sibling->child(current_side), colour::black);
				paint(sibling, colour::red);
				detail::rotate(root, sibling, sibling_side);
				sibling = parent->child(sibling_side);
			}

			paint(sibling, parent->augment);
			paint(parent, colour::black);
			paint(sibling->child(sibling_side), colour::black);
			detail::rotate(root, parent, current_side);
			current = root;
		}

		paint(current, colour::black);
	}

	template <class node_type>
	bool red_black::is_red(const node_type* subtree) noexcept
	{
		return subtree != nullptr && subtree->augment == colour::red;
	}

	template <class node_type>
	void red_black::paint(node_type* subtree, colour shade) noexcept
	{
		if (subtree != nullptr)
			subtree->augment = shade;
	}
}
