#include <gtest/gtest.h>
#include <functional>
#include <sstream>
#include <string>
#include <utility>
#include "linked_list.hpp"
#include "support/compound_type.hpp"
#include "support/test_values.hpp"
#include "support/throwing_copy.hpp"

namespace
{
	using ds::test::compound_type;
	using numbers_type = ds::linked_list<int>;
	using compounds_type = ds::linked_list<compound_type, ds::test::by_text>;

	class linked_list_fixture : public testing::Test
	{
	protected:
		numbers_type numbers{ ds::test::numbers, ds::test::number_count };
		compounds_type compounds{ ds::test::compounds, ds::test::compound_count };
	};

	std::string printed(const numbers_type& values)
	{
		std::ostringstream out;
		out << values;
		return out.str();
	}

	// Iterator to the element at the index, found by walking from the front.
	numbers_type::const_iterator position_of(const numbers_type& values, std::size_t index)
	{
		numbers_type::const_iterator current = values.begin();
		for (std::size_t step = 0; step < index; ++step)
			++current;
		return current;
	}
}

//------------------------------------------------
// Constructors and destructor.

TEST(linked_list, default_constructor_creates_an_empty_list)
{
	const numbers_type values;

	EXPECT_TRUE(values.empty());
	EXPECT_EQ(0u, values.size());
}

TEST(linked_list, initializer_list_constructor_keeps_the_order)
{
	const numbers_type values{ 3, 1, 2 };

	EXPECT_EQ("3 1 2", printed(values));
	EXPECT_EQ(2, values.back());
}

TEST_F(linked_list_fixture, raw_buffer_constructor_copies_every_element)
{
	std::size_t index = 0;
	for (const int value : numbers)
		EXPECT_EQ(ds::test::numbers[index++], value);

	EXPECT_EQ(ds::test::number_count, numbers.size());
}

TEST_F(linked_list_fixture, copy_constructor_makes_an_independent_copy)
{
	compounds_type copy(compounds);
	copy.remove(0);

	EXPECT_EQ(ds::test::compound_count, compounds.size());
	EXPECT_EQ(ds::test::compounds[1], copy.front());
}

TEST(linked_list, copy_constructor_releases_the_nodes_when_an_element_copy_throws)
{
	const ds::linked_list<ds::test::throwing_copy> source{ ds::test::throwing_copy(1), ds::test::throwing_copy(2) };
	const ds::test::copy_budget budget(1);

	EXPECT_THROW(ds::linked_list<ds::test::throwing_copy>{ source }, std::runtime_error);
}

TEST_F(linked_list_fixture, move_constructor_steals_the_nodes)
{
	const compounds_type moved(std::move(compounds));

	EXPECT_EQ(ds::test::compound_count, moved.size());
	EXPECT_TRUE(compounds.empty());
	EXPECT_FALSE(compounds.begin() != compounds.end());
}

//------------------------------------------------
// Modifier methods.

TEST_F(linked_list_fixture, copy_assignment_replaces_the_contents)
{
	numbers_type target{ 1, 2 };
	target = numbers;

	EXPECT_EQ(numbers, target);
	EXPECT_EQ(numbers.back(), target.back());
}

TEST_F(linked_list_fixture, move_assignment_steals_the_nodes)
{
	compounds_type target{ ds::test::compounds[0] };
	target = std::move(compounds);

	EXPECT_EQ(ds::test::compound_count, target.size());
	EXPECT_TRUE(compounds.empty());
}

TEST_F(linked_list_fixture, move_assignment_to_itself_keeps_the_contents)
{
	numbers_type& same = numbers;
	numbers = std::move(same);

	EXPECT_EQ(ds::test::number_count, numbers.size());
}

TEST_F(linked_list_fixture, clear_removes_every_element)
{
	compounds.clear();

	EXPECT_TRUE(compounds.empty());
	EXPECT_THROW(compounds.front(), ds::empty_error);
}

TEST(linked_list, set_compare_changes_the_order_used_by_the_queries)
{
	ds::linked_list<int, std::function<bool(const int&, const int&)>> values{ 2, 3, 1 };
	values.set_compare(std::greater<int>());

	EXPECT_EQ(3, *values.minimum());
	EXPECT_EQ(1, *values.maximum());
}

//------------------------------------------------
// Specific methods.

TEST(linked_list, insert_without_index_appends_at_the_tail)
{
	numbers_type values;
	values.insert(1).insert(2);

	EXPECT_EQ("1 2", printed(values));
	EXPECT_EQ(2, values.back());
}

TEST_F(linked_list_fixture, insert_at_an_index_links_the_node_there)
{
	numbers.insert(42, 0).insert(43, 5).insert(44, numbers.size());

	EXPECT_EQ(42, numbers.front());
	EXPECT_EQ(43, *position_of(numbers, 5));
	EXPECT_EQ(65535, *position_of(numbers, 6));
	EXPECT_EQ(44, numbers.back());
	EXPECT_EQ(ds::test::number_count + 3, numbers.size());
}

TEST_F(linked_list_fixture, insert_past_the_size_throws)
{
	EXPECT_THROW(numbers.insert(1, numbers.size() + 1), ds::index_error);
}

TEST_F(linked_list_fixture, insert_after_links_behind_the_position)
{
	numbers.insert_after(numbers.begin(), 42).insert_after(position_of(numbers, numbers.size() - 1), 43);

	EXPECT_EQ(42, *position_of(numbers, 1));
	EXPECT_EQ(43, numbers.back());
}

TEST_F(linked_list_fixture, insert_after_the_end_throws)
{
	EXPECT_THROW(numbers.insert_after(numbers.end(), 1), ds::position_error);
}

TEST_F(linked_list_fixture, remove_unlinks_front_middle_and_tail)
{
	numbers.remove(0).remove(numbers.size() - 1).remove(2);

	EXPECT_EQ("7 0 65535 -2147483648 -7 3 7", printed(numbers));
	EXPECT_EQ(7, numbers.back());
}

TEST_F(linked_list_fixture, remove_with_a_bad_index_throws)
{
	EXPECT_THROW(numbers.remove(numbers.size()), ds::index_error);
}

TEST(linked_list, removing_the_only_element_empties_both_ends)
{
	numbers_type values{ 1 };
	values.remove(0).insert(2);

	EXPECT_EQ(2, values.front());
	EXPECT_EQ(2, values.back());
}

TEST_F(linked_list_fixture, remove_after_unlinks_the_next_node_and_updates_the_tail)
{
	numbers.remove_after(numbers.begin()).remove_after(position_of(numbers, numbers.size() - 2));

	EXPECT_EQ(0, *position_of(numbers, 1));
	EXPECT_EQ(7, numbers.back());
}

TEST_F(linked_list_fixture, remove_after_the_last_element_or_the_end_throws)
{
	EXPECT_THROW(numbers.remove_after(position_of(numbers, numbers.size() - 1)), ds::position_error);
	EXPECT_THROW(numbers.remove_after(numbers.end()), ds::position_error);
}

//------------------------------------------------
// Constant methods.

TEST_F(linked_list_fixture, equality_compares_the_elements)
{
	numbers_type same(numbers);
	numbers_type different(numbers);
	*different.begin() = 100;

	EXPECT_TRUE(numbers == same);
	EXPECT_TRUE(numbers != different);
	EXPECT_TRUE(numbers != numbers_type{ 1 });
}

TEST_F(linked_list_fixture, print_writes_the_elements_and_a_newline)
{
	const numbers_type values{ 1, -2, 3 };
	testing::internal::CaptureStdout();
	values.print();

	EXPECT_EQ("1 -2 3\n", testing::internal::GetCapturedStdout());
}

TEST_F(linked_list_fixture, front_and_back_read_both_ends)
{
	EXPECT_EQ(-1, numbers.front());
	EXPECT_EQ(INT_MAX, numbers.back());
}

TEST(linked_list, front_and_back_of_an_empty_list_throw)
{
	const numbers_type values;

	EXPECT_THROW(values.front(), ds::empty_error);
	EXPECT_THROW(values.back(), ds::empty_error);
}

//------------------------------------------------
// Query operations.

TEST_F(linked_list_fixture, search_finds_the_first_match_or_returns_the_end)
{
	EXPECT_FALSE(numbers.search(7) != position_of(numbers, 1));
	EXPECT_FALSE(numbers.search(8) != numbers.end());
}

TEST_F(linked_list_fixture, minimum_and_maximum_return_the_extreme_elements)
{
	EXPECT_EQ(INT_MIN, *numbers.minimum());
	EXPECT_EQ(INT_MAX, *numbers.maximum());
	EXPECT_EQ(ds::test::compounds[1], *compounds.minimum());
	EXPECT_EQ(ds::test::compounds[4], *compounds.maximum());
}

TEST(linked_list, minimum_and_maximum_of_an_empty_list_throw)
{
	const numbers_type values;

	EXPECT_THROW(values.minimum(), ds::empty_error);
	EXPECT_THROW(values.maximum(), ds::empty_error);
}

TEST_F(linked_list_fixture, predecessor_and_successor_return_the_closest_neighbours)
{
	const numbers_type::const_iterator seven = position_of(numbers, 1);

	EXPECT_EQ(3, *numbers.predecessor(seven));
	EXPECT_EQ(255, *numbers.successor(seven));
	EXPECT_FALSE(numbers.predecessor(numbers.minimum()) != numbers.end());
	EXPECT_FALSE(numbers.successor(numbers.maximum()) != numbers.end());
}

//------------------------------------------------
// Iterator methods.

TEST_F(linked_list_fixture, mutable_iteration_writes_the_elements)
{
	for (int& value : numbers)
		value = 1;

	EXPECT_EQ(1, numbers.front());
	EXPECT_FALSE(numbers.search(7) != numbers.end());
}

TEST(linked_list, an_empty_list_begins_at_its_end)
{
	const numbers_type values;

	EXPECT_FALSE(values.begin() != values.end());
}

//------------------------------------------------
// Friend functions.

TEST_F(linked_list_fixture, stream_output_separates_elements_with_spaces)
{
	EXPECT_EQ("-1 7 0 255 65535 -2147483648 -7 3 7 2147483647", printed(numbers));
	EXPECT_EQ("", printed(numbers_type()));
}

TEST_F(linked_list_fixture, convert_copies_into_a_new_block)
{
	const auto block = convert(compounds);

	for (std::size_t index = 0; index < ds::test::compound_count; ++index)
		EXPECT_EQ(ds::test::compounds[index], block[index]);
}
