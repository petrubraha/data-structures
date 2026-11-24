#include <gtest/gtest.h>
#include <utility>
#include "queue.hpp"
#include "support/compound_type.hpp"
#include "support/test_values.hpp"

namespace
{
	using ds::test::compound_type;

	class queue_fixture : public testing::Test
	{
	protected:
		ds::queue<int> numbers{ ds::test::numbers, ds::test::number_count };
		ds::queue<compound_type> compounds{ ds::test::compounds, ds::test::compound_count };
	};
}

TEST(queue, default_constructor_creates_an_empty_queue)
{
	const ds::queue<int> values;

	EXPECT_TRUE(values.empty());
	EXPECT_EQ(0u, values.size());
}

TEST(queue, initializer_list_constructor_puts_the_first_element_in_front)
{
	const ds::queue<int> values{ 100, 60, 42 };

	EXPECT_EQ(100, values.front());
	EXPECT_EQ(42, values.back());
}

TEST_F(queue_fixture, raw_buffer_constructor_keeps_the_order)
{
	EXPECT_EQ(ds::test::numbers[0], numbers.front());
	EXPECT_EQ(ds::test::numbers[ds::test::number_count - 1], numbers.back());
	EXPECT_EQ(ds::test::compounds[0], compounds.front());
}

TEST_F(queue_fixture, copy_makes_an_independent_queue)
{
	ds::queue<compound_type> copy(compounds);
	copy.pop();

	EXPECT_EQ(ds::test::compound_count, compounds.size());
	EXPECT_EQ(ds::test::compounds[1], copy.front());
}

TEST_F(queue_fixture, move_steals_the_elements)
{
	ds::queue<compound_type> target;
	target = std::move(compounds);
	const ds::queue<compound_type> moved(std::move(target));

	EXPECT_EQ(ds::test::compound_count, moved.size());
	EXPECT_TRUE(target.empty());
}

TEST_F(queue_fixture, copy_assignment_replaces_the_contents)
{
	ds::queue<int> target{ 1 };
	target = numbers;

	EXPECT_TRUE(target == numbers);
}

TEST(queue, push_and_pop_work_first_in_first_out)
{
	ds::queue<int> values{ 100, 60, 42, 25 };
	values.pop().pop().push(15).push(10).pop();

	EXPECT_EQ(25, values.front());
	EXPECT_EQ(10, values.back());
	EXPECT_EQ(3u, values.size());
}

TEST(queue, draining_and_refilling_keeps_both_ends_consistent)
{
	ds::queue<int> values{ 1 };
	values.pop().push(2);

	EXPECT_EQ(2, values.front());
	EXPECT_EQ(2, values.back());
}

TEST(queue, pop_front_and_back_of_an_empty_queue_throw)
{
	ds::queue<int> values;

	EXPECT_THROW(values.pop(), ds::empty_error);
	EXPECT_THROW(values.front(), ds::empty_error);
	EXPECT_THROW(values.back(), ds::empty_error);
}

TEST_F(queue_fixture, equality_compares_the_elements_in_order)
{
	ds::queue<int> same(numbers);
	ds::queue<int> different(numbers);
	different.pop().push(0);

	EXPECT_TRUE(numbers == same);
	EXPECT_TRUE(numbers != different);
}
