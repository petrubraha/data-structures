#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <utility>
#include "deque.hpp"
#include "support/compound_type.hpp"
#include "support/test_values.hpp"
#include "support/throwing_copy.hpp"

namespace
{
	using ds::test::compound_type;

	class deque_fixture : public testing::Test
	{
	protected:
		ds::deque<int> numbers{ ds::test::numbers, ds::test::number_count };
		ds::deque<compound_type> compounds{ ds::test::compounds, ds::test::compound_count };
	};

	std::string printed(const ds::deque<int>& values)
	{
		std::ostringstream out;
		out << values;
		return out.str();
	}
}

//------------------------------------------------
// Constructors and destructor.

TEST(deque, default_constructor_creates_an_empty_deque)
{
	const ds::deque<int> values;

	EXPECT_TRUE(values.empty());
	EXPECT_EQ(0u, values.size());
}

TEST(deque, initializer_list_constructor_keeps_the_order)
{
	const ds::deque<int> values{ 3, 1, 2 };

	EXPECT_EQ("3 1 2", printed(values));
}

TEST_F(deque_fixture, raw_buffer_constructor_copies_every_element)
{
	std::size_t index = 0;
	for (const int value : numbers)
		EXPECT_EQ(ds::test::numbers[index++], value);

	EXPECT_EQ(ds::test::number_count, numbers.size());
}

TEST_F(deque_fixture, copy_constructor_makes_an_independent_copy)
{
	ds::deque<compound_type> copy(compounds);
	copy.pop_front();

	EXPECT_EQ(ds::test::compound_count, compounds.size());
	EXPECT_EQ(ds::test::compounds[1], copy.front());
}

TEST(deque, copy_constructor_releases_the_nodes_when_an_element_copy_throws)
{
	const ds::deque<ds::test::throwing_copy> source{ ds::test::throwing_copy(1), ds::test::throwing_copy(2) };
	const ds::test::copy_budget budget(1);

	EXPECT_THROW(ds::deque<ds::test::throwing_copy>{ source }, std::runtime_error);
}

TEST_F(deque_fixture, move_constructor_steals_the_nodes)
{
	const ds::deque<compound_type> moved(std::move(compounds));

	EXPECT_EQ(ds::test::compound_count, moved.size());
	EXPECT_TRUE(compounds.empty());
}

//------------------------------------------------
// Modifier methods.

TEST_F(deque_fixture, copy_assignment_replaces_the_contents)
{
	ds::deque<int> target{ 1, 2 };
	target = numbers;

	EXPECT_TRUE(target == numbers);
	EXPECT_EQ(numbers.back(), target.back());
}

TEST_F(deque_fixture, move_assignment_steals_the_nodes)
{
	ds::deque<compound_type> target{ ds::test::compounds[0] };
	target = std::move(compounds);

	EXPECT_EQ(ds::test::compound_count, target.size());
	EXPECT_TRUE(compounds.empty());
}

TEST_F(deque_fixture, move_assignment_to_itself_keeps_the_contents)
{
	ds::deque<int>& same = numbers;
	numbers = std::move(same);

	EXPECT_EQ(ds::test::number_count, numbers.size());
}

TEST_F(deque_fixture, clear_removes_every_element)
{
	compounds.clear();

	EXPECT_TRUE(compounds.empty());
	EXPECT_FALSE(compounds.begin() != compounds.end());
}

//------------------------------------------------
// Specific methods.

TEST(deque, push_links_at_both_ends)
{
	ds::deque<int> values;
	values.push_back(2).push_front(1).push_back(3).push_front(0);

	EXPECT_EQ("0 1 2 3", printed(values));
	EXPECT_EQ(0, values.front());
	EXPECT_EQ(3, values.back());
}

TEST_F(deque_fixture, pop_unlinks_at_both_ends)
{
	numbers.pop_front().pop_back().pop_front();

	EXPECT_EQ(0, numbers.front());
	EXPECT_EQ(7, numbers.back());
	EXPECT_EQ(ds::test::number_count - 3, numbers.size());
}

TEST(deque, popping_the_only_element_from_either_end_empties_the_deque)
{
	ds::deque<int> values{ 1 };
	values.pop_back().push_front(2).pop_front().push_back(3);

	EXPECT_EQ(3, values.front());
	EXPECT_EQ(3, values.back());
	EXPECT_EQ(1u, values.size());
}

TEST(deque, pop_of_an_empty_deque_throws)
{
	ds::deque<int> values;

	EXPECT_THROW(values.pop_front(), ds::empty_error);
	EXPECT_THROW(values.pop_back(), ds::empty_error);
}

//------------------------------------------------
// Constant methods.

TEST_F(deque_fixture, equality_compares_the_elements_in_order)
{
	ds::deque<int> same(numbers);
	ds::deque<int> different(numbers);
	different.pop_back().push_back(0);

	EXPECT_TRUE(numbers == same);
	EXPECT_TRUE(numbers != different);
	EXPECT_TRUE(numbers != ds::deque<int>{ 1 });
}

TEST_F(deque_fixture, front_and_back_read_both_ends)
{
	EXPECT_EQ(-1, numbers.front());
	EXPECT_EQ(INT_MAX, numbers.back());
	EXPECT_EQ(ds::test::compounds[ds::test::compound_count - 1], compounds.back());
}

TEST(deque, front_and_back_of_an_empty_deque_throw)
{
	const ds::deque<int> values;

	EXPECT_THROW(values.front(), ds::empty_error);
	EXPECT_THROW(values.back(), ds::empty_error);
}

//------------------------------------------------
// Iterator methods.

TEST_F(deque_fixture, mutable_iteration_writes_the_elements)
{
	for (int& value : numbers)
		value = 1;

	EXPECT_EQ(1, numbers.front());
	EXPECT_EQ(1, numbers.back());
}

//------------------------------------------------
// Friend functions.

TEST_F(deque_fixture, stream_output_separates_elements_with_spaces)
{
	EXPECT_EQ("-1 7 0 255 65535 -2147483648 -7 3 7 2147483647", printed(numbers));
	EXPECT_EQ("", printed(ds::deque<int>()));
}

TEST_F(deque_fixture, convert_copies_into_a_new_block)
{
	const auto block = convert(compounds);

	for (std::size_t index = 0; index < ds::test::compound_count; ++index)
		EXPECT_EQ(ds::test::compounds[index], block[index]);
}
