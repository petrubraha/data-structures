#include <gtest/gtest.h>
#include <functional>
#include <sstream>
#include <utility>
#include "array.hpp"
#include "support/compound_type.hpp"
#include "support/test_values.hpp"

namespace
{
	using ds::test::compound_type;
	using numbers_type = ds::array<int>;
	using compounds_type = ds::array<compound_type, ds::test::by_number>;

	constexpr std::size_t npos = numbers_type::npos;

	class array_fixture : public testing::Test
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
}

//------------------------------------------------
// Constructors and destructor.

TEST(array, default_constructor_creates_an_empty_array_without_storage)
{
	const numbers_type values;

	EXPECT_TRUE(values.empty());
	EXPECT_EQ(0u, values.capacity());
}

TEST(array, initializer_list_constructor_keeps_the_order)
{
	const numbers_type values{ 3, 1, 2 };

	EXPECT_EQ("3 1 2", printed(values));
}

TEST(array, capacity_constructor_reserves_without_elements)
{
	const numbers_type values(32);

	EXPECT_TRUE(values.empty());
	EXPECT_EQ(32u, values.capacity());
}

TEST(array, capacity_constructor_rejects_an_impossible_capacity)
{
	EXPECT_THROW(numbers_type(static_cast<std::size_t>(-1)), ds::capacity_error);
}

TEST_F(array_fixture, raw_buffer_constructor_copies_every_element)
{
	ASSERT_EQ(ds::test::number_count, numbers.size());

	for (std::size_t index = 0; index < ds::test::number_count; ++index)
		EXPECT_EQ(ds::test::numbers[index], numbers.get(index));
}

TEST_F(array_fixture, copy_constructor_makes_an_independent_copy)
{
	compounds_type copy(compounds);
	copy.remove(0);

	EXPECT_EQ(ds::test::compound_count, compounds.size());
	EXPECT_EQ(ds::test::compounds[1], copy.get(0));
}

TEST_F(array_fixture, move_constructor_steals_the_storage)
{
	const compounds_type moved(std::move(compounds));

	EXPECT_EQ(ds::test::compound_count, moved.size());
	EXPECT_TRUE(compounds.empty());
	EXPECT_EQ(0u, compounds.capacity());
}

//------------------------------------------------
// Modifier methods.

TEST_F(array_fixture, copy_assignment_replaces_the_contents)
{
	numbers_type target{ 1, 2 };
	target = numbers;

	EXPECT_EQ(numbers, target);
}

TEST_F(array_fixture, copy_assignment_to_itself_keeps_the_contents)
{
	const numbers_type expected(numbers);
	numbers = *&numbers;

	EXPECT_EQ(expected, numbers);
}

TEST_F(array_fixture, move_assignment_steals_the_storage)
{
	compounds_type target{ ds::test::compounds[0] };
	target = std::move(compounds);

	EXPECT_EQ(ds::test::compound_count, target.size());
	EXPECT_TRUE(compounds.empty());
}

TEST_F(array_fixture, clear_removes_every_element_and_keeps_the_capacity)
{
	const std::size_t reserved = compounds.capacity();
	compounds.clear();

	EXPECT_TRUE(compounds.empty());
	EXPECT_EQ(reserved, compounds.capacity());
}

TEST(array, set_compare_changes_the_order_used_by_sort_and_queries)
{
	ds::array<int, std::function<bool(const int&, const int&)>> values{ 2, 3, 1 };
	values.set_compare(std::greater<int>());
	values.sort();

	EXPECT_EQ(3, values.get(0));
	EXPECT_EQ(1, values.get(2));
	EXPECT_EQ(0u, values.minimum());
}

//------------------------------------------------
// Specific methods.

TEST(array, insert_without_index_appends)
{
	numbers_type values;
	values.insert(1).insert(2);

	EXPECT_EQ("1 2", printed(values));
}

TEST_F(array_fixture, insert_at_an_index_shifts_the_tail_right)
{
	numbers.insert(42, 0).insert(43, 5).insert(44, numbers.size());

	EXPECT_EQ(42, numbers.get(0));
	EXPECT_EQ(43, numbers.get(5));
	EXPECT_EQ(65535, numbers.get(6));
	EXPECT_EQ(44, numbers.get(numbers.size() - 1));
	EXPECT_EQ(ds::test::number_count + 3, numbers.size());
}

TEST_F(array_fixture, insert_past_the_size_throws)
{
	EXPECT_THROW(numbers.insert(1, numbers.size() + 1), ds::index_error);
}

TEST(array, insert_grows_the_storage_and_keeps_every_element)
{
	compounds_type values;
	for (const compound_type& value : ds::test::compounds)
		for (std::size_t repeat = 0; repeat < numbers_type::default_capacity; ++repeat)
			values.insert(value, 0);

	EXPECT_EQ(ds::test::compound_count * numbers_type::default_capacity, values.size());
	EXPECT_GE(values.capacity(), values.size());
	EXPECT_EQ(ds::test::compounds[ds::test::compound_count - 1], values.get(0));
}

TEST_F(array_fixture, remove_shifts_the_tail_left)
{
	numbers.remove(0).remove(numbers.size() - 1).remove(2);

	EXPECT_EQ("7 0 65535 -2147483648 -7 3 7", printed(numbers));
}

TEST_F(array_fixture, remove_with_a_bad_index_throws)
{
	EXPECT_THROW(numbers.remove(numbers.size()), ds::index_error);
}

TEST_F(array_fixture, sort_orders_by_the_comparator)
{
	numbers.sort();

	EXPECT_EQ(numbers_type(ds::test::sorted_numbers, ds::test::number_count), numbers);
}

TEST_F(array_fixture, sort_accepts_every_algorithm_policy)
{
	const numbers_type expected(ds::test::sorted_numbers, ds::test::number_count);

	EXPECT_EQ(expected, numbers_type(numbers).sort<ds::bubble_sort>());
	EXPECT_EQ(expected, numbers_type(numbers).sort<ds::selection_sort>());
	EXPECT_EQ(expected, numbers_type(numbers).sort<ds::insertion_sort>());
	EXPECT_EQ(expected, numbers_type(numbers).sort<ds::merge_sort>());
	EXPECT_EQ(expected, numbers_type(numbers).sort<ds::heap_sort>());
	EXPECT_EQ(expected, numbers_type(numbers).sort<ds::quick_sort>());
}

TEST_F(array_fixture, sort_orders_compound_values_by_a_custom_comparator)
{
	compounds.sort<ds::merge_sort>();

	EXPECT_EQ(12, compounds.get(0).get_number());
	EXPECT_EQ(ds::test::compounds[1], compounds.get(1));
	EXPECT_EQ(ds::test::compounds[2], compounds.get(2));
	EXPECT_EQ(20, compounds.get(4).get_number());
}

//------------------------------------------------
// Constant methods.

TEST_F(array_fixture, equality_compares_the_elements)
{
	numbers_type same(numbers);
	numbers_type different(numbers);
	different[0] = 100;

	EXPECT_TRUE(numbers == same);
	EXPECT_TRUE(numbers != different);
	EXPECT_TRUE(numbers != numbers_type{ 1 });
}

TEST_F(array_fixture, print_writes_the_elements_and_a_newline)
{
	const numbers_type values{ 1, -2, 3 };
	testing::internal::CaptureStdout();
	values.print();

	EXPECT_EQ("1 -2 3\n", testing::internal::GetCapturedStdout());
}

TEST_F(array_fixture, element_access_reads_and_writes_by_index)
{
	numbers[1] = 70;
	const numbers_type& view = numbers;

	EXPECT_EQ(70, numbers.get(1));
	EXPECT_EQ(70, view[1]);
}

TEST_F(array_fixture, element_access_with_a_bad_index_throws)
{
	const numbers_type& view = numbers;

	EXPECT_THROW(numbers.get(numbers.size()), ds::index_error);
	EXPECT_THROW(numbers[numbers.size()], ds::index_error);
	EXPECT_THROW(view[numbers.size()], ds::index_error);
}

//------------------------------------------------
// Query operations.

TEST_F(array_fixture, search_finds_the_first_match_or_returns_npos)
{
	EXPECT_EQ(1u, numbers.search(7));
	EXPECT_EQ(npos, numbers.search(8));
}

TEST_F(array_fixture, minimum_and_maximum_return_the_extreme_indices)
{
	EXPECT_EQ(5u, numbers.minimum());
	EXPECT_EQ(9u, numbers.maximum());
	EXPECT_EQ(4u, compounds.minimum());
	EXPECT_EQ(0u, compounds.maximum());
}

TEST(array, minimum_and_maximum_of_an_empty_array_throw)
{
	const numbers_type values;

	EXPECT_THROW(values.minimum(), ds::empty_error);
	EXPECT_THROW(values.maximum(), ds::empty_error);
}

TEST_F(array_fixture, predecessor_and_successor_return_the_closest_neighbours)
{
	EXPECT_EQ(7u, numbers.predecessor(1));
	EXPECT_EQ(3u, numbers.successor(1));
	EXPECT_EQ(npos, numbers.predecessor(5));
	EXPECT_EQ(npos, numbers.successor(9));
}

TEST_F(array_fixture, predecessor_and_successor_with_a_bad_index_throw)
{
	EXPECT_THROW(numbers.predecessor(numbers.size()), ds::index_error);
	EXPECT_THROW(numbers.successor(numbers.size()), ds::index_error);
}

//------------------------------------------------
// Iterator methods.

TEST_F(array_fixture, iteration_visits_every_element_in_order)
{
	std::size_t index = 0;
	for (const int value : numbers)
		EXPECT_EQ(ds::test::numbers[index++], value);

	EXPECT_EQ(ds::test::number_count, index);
}

TEST_F(array_fixture, mutable_iteration_writes_the_elements)
{
	for (int& value : numbers)
		value = 1;

	EXPECT_EQ(0u, numbers.search(1));
	EXPECT_EQ(npos, numbers.search(7));
}

TEST(array, an_empty_array_begins_at_its_end)
{
	const numbers_type values;

	EXPECT_FALSE(values.begin() != values.end());
}

//------------------------------------------------
// Friend functions.

TEST_F(array_fixture, stream_output_separates_elements_with_spaces)
{
	EXPECT_EQ("-1 7 0 255 65535 -2147483648 -7 3 7 2147483647", printed(numbers));
	EXPECT_EQ("", printed(numbers_type()));
}

TEST_F(array_fixture, convert_copies_into_a_new_block)
{
	const auto block = convert(compounds);

	for (std::size_t index = 0; index < ds::test::compound_count; ++index)
		EXPECT_EQ(ds::test::compounds[index], block[index]);
}
