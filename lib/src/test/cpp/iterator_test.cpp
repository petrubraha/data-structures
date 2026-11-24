#include <gtest/gtest.h>
#include "iterator/array_iterator.hpp"
#include "iterator/list_iterator.hpp"
#include "iterator/tree_iterator.hpp"
#include "node/deque_node.hpp"
#include "node/list_node.hpp"
#include "node/tree_node.hpp"

namespace
{
	using list_node = ds::detail::list_node<int>;
	using deque_node = ds::detail::deque_node<int>;
	using tree_node = ds::detail::tree_node<int, int>;
	using list_reader = ds::detail::list_iterator<list_node, const int>;
	using tree_reader = ds::detail::tree_iterator<tree_node, const int>;
	using ds::detail::side;

	constexpr int element_count = 3;
}

TEST(array_iterator, walks_every_element_in_order)
{
	int values[element_count] = { 1, 2, 3 };
	ds::detail::array_iterator<int> current(values, values + element_count);
	const ds::detail::array_iterator<int> end(values + element_count, values + element_count);

	EXPECT_EQ(1, *current);
	EXPECT_EQ(2, *++current);
	EXPECT_EQ(3, *++current);
	EXPECT_FALSE(++current != end);
}

TEST(array_iterator, writes_through_a_mutable_iterator)
{
	int values[element_count] = { 1, 2, 3 };
	const ds::detail::array_iterator<int> current(values, values + element_count);
	*current = 9;

	EXPECT_EQ(9, values[0]);
}

TEST(list_iterator, follows_next_links_of_list_and_deque_nodes)
{
	list_node last(2);
	list_node first(1);
	first.next = &last;
	deque_node only(5);
	list_reader list_current(&first);
	const ds::detail::list_iterator<deque_node, int> deque_current(&only);

	EXPECT_EQ(1, *list_current);
	EXPECT_EQ(2, *++list_current);
	EXPECT_FALSE(++list_current != list_reader(nullptr));
	EXPECT_EQ(5, *deque_current);
}

TEST(list_iterator, converts_from_mutable_to_read_only)
{
	list_node only(3);
	const ds::detail::list_iterator<list_node, int> writer(&only);
	const list_reader reader = writer;

	EXPECT_EQ(3, *reader);
	EXPECT_FALSE(reader != writer);
}

TEST(array_iterator, converts_from_mutable_to_read_only)
{
	int values[element_count] = { 4, 5, 6 };
	const ds::detail::array_iterator<int> writer(values, values + element_count);
	const ds::detail::array_iterator<const int> reader = writer;

	EXPECT_EQ(4, *reader);
	EXPECT_FALSE(reader != writer);
}

TEST(tree_iterator, visits_nodes_in_order)
{
	tree_node root(2);
	tree_node left(1);
	tree_node right(3);
	root.child(side::left) = &left;
	root.child(side::right) = &right;
	left.parent = &root;
	right.parent = &root;
	tree_reader current(&left);

	EXPECT_EQ(1, *current);
	EXPECT_EQ(2, *++current);
	EXPECT_EQ(3, *++current);
	EXPECT_FALSE(++current != tree_reader(nullptr));
}

TEST(iterator_death_test, dereferencing_or_advancing_past_the_end_terminates)
{
	int values[1] = { 1 };
	ds::detail::array_iterator<int> array_end(values + 1, values + 1);
	ds::detail::list_iterator<list_node, int> list_end(nullptr);
	ds::detail::tree_iterator<tree_node, int> tree_end(nullptr);

	EXPECT_DEATH(*array_end, "array_iterator::operator\\*: dereferenced past the end");
	EXPECT_DEATH(++array_end, "array_iterator::operator\\+\\+: advanced past the end");
	EXPECT_DEATH(*list_end, "list_iterator::operator\\*: dereferenced past the end");
	EXPECT_DEATH(++list_end, "list_iterator::operator\\+\\+: advanced past the end");
	EXPECT_DEATH(*tree_end, "tree_iterator::operator\\*: dereferenced past the end");
	EXPECT_DEATH(++tree_end, "tree_iterator::operator\\+\\+: advanced past the end");
}
