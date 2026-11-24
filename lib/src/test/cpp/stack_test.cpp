#include <gtest/gtest.h>
#include <utility>
#include "stack.hpp"
#include "support/compound_type.hpp"
#include "support/test_values.hpp"

namespace
{
	using ds::test::compound_type;

	class stack_fixture : public testing::Test
	{
	protected:
		ds::stack<int> numbers{ ds::test::numbers, ds::test::number_count };
		ds::stack<compound_type> compounds{ ds::test::compounds, ds::test::compound_count };
	};
}

TEST(stack, default_constructor_creates_an_empty_stack)
{
	const ds::stack<int> values;

	EXPECT_TRUE(values.empty());
	EXPECT_EQ(0u, values.size());
}

TEST(stack, initializer_list_constructor_puts_the_last_element_on_top)
{
	const ds::stack<int> values{ 1, 2, 3 };

	EXPECT_EQ(3, values.top());
	EXPECT_EQ(3u, values.size());
}

TEST_F(stack_fixture, raw_buffer_constructor_puts_the_last_element_on_top)
{
	EXPECT_EQ(ds::test::numbers[ds::test::number_count - 1], numbers.top());
	EXPECT_EQ(ds::test::compounds[ds::test::compound_count - 1], compounds.top());
}

TEST_F(stack_fixture, copy_makes_an_independent_stack)
{
	ds::stack<compound_type> copy(compounds);
	copy.pop();

	EXPECT_EQ(ds::test::compound_count, compounds.size());
	EXPECT_EQ(ds::test::compound_count - 1, copy.size());
}

TEST_F(stack_fixture, move_steals_the_elements)
{
	ds::stack<compound_type> target;
	target = std::move(compounds);
	const ds::stack<compound_type> moved(std::move(target));

	EXPECT_EQ(ds::test::compound_count, moved.size());
	EXPECT_TRUE(target.empty());
}

TEST_F(stack_fixture, copy_assignment_replaces_the_contents)
{
	ds::stack<int> target{ 1 };
	target = numbers;

	EXPECT_TRUE(target == numbers);
}

TEST(stack, push_and_pop_work_last_in_first_out)
{
	ds::stack<int> values;
	values.push(1).push(2).push(3).pop().pop().push(4);

	EXPECT_EQ(4, values.top());
	values.pop();
	EXPECT_EQ(1, values.top());
	EXPECT_EQ(1u, values.size());
}

TEST(stack, pop_and_top_of_an_empty_stack_throw)
{
	ds::stack<int> values;

	EXPECT_THROW(values.pop(), ds::empty_error);
	EXPECT_THROW(values.top(), ds::empty_error);
}

TEST_F(stack_fixture, equality_compares_the_elements_in_order)
{
	ds::stack<int> same(numbers);
	ds::stack<int> different(numbers);
	different.pop().push(0);

	EXPECT_TRUE(numbers == same);
	EXPECT_TRUE(numbers != different);
}
