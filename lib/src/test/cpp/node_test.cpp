#include <gtest/gtest.h>
#include <string>
#include "detail/node/deque_node.hpp"
#include "detail/node/list_node.hpp"
#include "detail/node/tree_node.hpp"

namespace
{
	using tree_node = ds::detail::tree_node<int, int>;
	using ds::detail::side;

	// Builds the tree 20(10(5, 15), 30) with parent links.
	class tree_node_fixture : public testing::Test
	{
	protected:
		tree_node root_node{ 20 };
		tree_node left_node{ 10 };
		tree_node right_node{ 30 };
		tree_node left_left_node{ 5 };
		tree_node left_right_node{ 15 };
		tree_node* root = &root_node;

		tree_node_fixture()
		{
			attach(root_node, left_node, side::left);
			attach(root_node, right_node, side::right);
			attach(left_node, left_left_node, side::left);
			attach(left_node, left_right_node, side::right);
		}

		static void attach(tree_node& parent, tree_node& child, side direction)
		{
			parent.child(direction) = &child;
			child.parent = &parent;
		}
	};
}

TEST(list_node, holds_and_replaces_its_value)
{
	ds::detail::list_node<std::string> text("first");
	text.set("second");

	EXPECT_EQ("second", text.get());
	EXPECT_EQ(nullptr, text.next);
}

TEST(deque_node, starts_unlinked_on_both_sides)
{
	const ds::detail::deque_node<int> number(4);

	EXPECT_EQ(4, number.get());
	EXPECT_EQ(nullptr, number.previous);
	EXPECT_EQ(nullptr, number.next);
}

TEST(tree_node, opposite_swaps_the_side)
{
	static_assert(ds::detail::opposite(side::left) == side::right);
	static_assert(ds::detail::opposite(side::right) == side::left);
}

TEST_F(tree_node_fixture, side_in_parent_names_the_hanging_side)
{
	EXPECT_EQ(side::left, ds::detail::side_in_parent(&left_node));
	EXPECT_EQ(side::right, ds::detail::side_in_parent(&left_right_node));
}

TEST_F(tree_node_fixture, extreme_reaches_the_minimum_and_maximum)
{
	EXPECT_EQ(&left_left_node, ds::detail::extreme(root, side::left));
	EXPECT_EQ(&right_node, ds::detail::extreme(root, side::right));
}

TEST_F(tree_node_fixture, neighbor_walks_in_order_both_ways)
{
	EXPECT_EQ(&left_right_node, ds::detail::neighbor(&left_node, side::right));
	EXPECT_EQ(&root_node, ds::detail::neighbor(&left_right_node, side::right));
	EXPECT_EQ(nullptr, ds::detail::neighbor(&right_node, side::right));
	EXPECT_EQ(&left_right_node, ds::detail::neighbor(&root_node, side::left));
	EXPECT_EQ(nullptr, ds::detail::neighbor(&left_left_node, side::left));
}

TEST_F(tree_node_fixture, rotate_right_lifts_the_left_child)
{
	ds::detail::rotate(root, &root_node, side::right);

	EXPECT_EQ(&left_node, root);
	EXPECT_EQ(nullptr, left_node.parent);
	EXPECT_EQ(&root_node, left_node.child(side::right));
	EXPECT_EQ(&left_right_node, root_node.child(side::left));
	EXPECT_EQ(&root_node, left_right_node.parent);
}

TEST_F(tree_node_fixture, rotate_left_lifts_the_right_child_below_a_parent)
{
	ds::detail::rotate(root, &left_node, side::left);

	EXPECT_EQ(&root_node, root);
	EXPECT_EQ(&left_right_node, root_node.child(side::left));
	EXPECT_EQ(&left_node, left_right_node.child(side::left));
	EXPECT_EQ(nullptr, left_node.child(side::right));
}

TEST_F(tree_node_fixture, transplant_replaces_a_subtree)
{
	ds::detail::transplant(root, &left_node, &left_right_node);

	EXPECT_EQ(&left_right_node, root_node.child(side::left));
	EXPECT_EQ(&root_node, left_right_node.parent);
}

TEST_F(tree_node_fixture, transplant_of_the_root_with_null_empties_the_tree)
{
	ds::detail::transplant(root, &root_node, static_cast<tree_node*>(nullptr));

	EXPECT_EQ(nullptr, root);
}
