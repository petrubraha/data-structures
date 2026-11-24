#include <gtest/gtest.h>
#include <algorithm>
#include <functional>
#include <random>
#include <vector>
#include "array.hpp"
#include "sorting/bubble_sort.hpp"
#include "sorting/heap_sort.hpp"
#include "sorting/insertion_sort.hpp"
#include "sorting/merge_sort.hpp"
#include "sorting/quick_sort.hpp"
#include "sorting/selection_sort.hpp"
#include "support/compound_type.hpp"
#include "support/test_values.hpp"

namespace
{
	using ds::test::compound_type;

	constexpr std::size_t large_length = 1000;
	constexpr unsigned large_seed = 20240601;
	constexpr int large_value_range = 50;
	constexpr std::size_t stability_length = 64;
	constexpr std::size_t flag_period = 3;

	template <class algorithm>
	std::vector<int> sorted_by(std::vector<int> values)
	{
		algorithm::apply(values, values.size(), std::less<int>());
		return values;
	}

	std::vector<int> expected_for(std::vector<int> values)
	{
		std::sort(values.begin(), values.end());
		return values;
	}

	std::vector<int> large_input()
	{
		std::mt19937 generator(large_seed);
		std::uniform_int_distribution<int> distribution(-large_value_range, large_value_range);
		std::vector<int> values(large_length);
		for (int& value : values)
			value = distribution(generator);
		return values;
	}

	// Elements numbered by their original position, with flags repeating so equal keys interleave.
	std::vector<compound_type> stability_input()
	{
		std::vector<compound_type> values;
		for (std::size_t position = 0; position < stability_length; ++position)
			values.emplace_back(static_cast<int>(position), position % flag_period == 0, "stability element");
		return values;
	}

	template <class algorithm>
	class sorting_algorithm : public testing::Test {};

	template <class algorithm>
	class stable_sorting_algorithm : public testing::Test {};

	template <class pivot>
	class quick_sort_pivot : public testing::Test {};

	using algorithms = testing::Types<ds::bubble_sort, ds::selection_sort, ds::insertion_sort,
		ds::merge_sort, ds::heap_sort, ds::quick_sort>;
	using stable_algorithms = testing::Types<ds::bubble_sort, ds::insertion_sort, ds::merge_sort>;
	using pivots = testing::Types<ds::median_of_three, ds::first, ds::last, ds::middle, ds::random>;
}

TYPED_TEST_SUITE(sorting_algorithm, algorithms);
TYPED_TEST_SUITE(stable_sorting_algorithm, stable_algorithms);
TYPED_TEST_SUITE(quick_sort_pivot, pivots);

TYPED_TEST(sorting_algorithm, keeps_an_ordered_input)
{
	const std::vector<int> ordered{ -3, -1, 0, 2, 5, 8 };

	EXPECT_EQ(ordered, sorted_by<TypeParam>(ordered));
}

TYPED_TEST(sorting_algorithm, reverses_a_reversed_input)
{
	const std::vector<int> reversed{ 8, 5, 2, 0, -1, -3 };

	EXPECT_EQ(expected_for(reversed), sorted_by<TypeParam>(reversed));
}

TYPED_TEST(sorting_algorithm, groups_duplicates)
{
	const std::vector<int> duplicates{ 4, 1, 4, 4, 1, 0, 4, 0 };

	EXPECT_EQ(expected_for(duplicates), sorted_by<TypeParam>(duplicates));
}

TYPED_TEST(sorting_algorithm, leaves_single_and_empty_inputs_alone)
{
	EXPECT_EQ(std::vector<int>{ 7 }, sorted_by<TypeParam>({ 7 }));
	EXPECT_EQ(std::vector<int>{}, sorted_by<TypeParam>({}));
}

TYPED_TEST(sorting_algorithm, sorts_the_shared_test_values)
{
	const std::vector<int> values(std::begin(ds::test::numbers), std::end(ds::test::numbers));
	const std::vector<int> expected(std::begin(ds::test::sorted_numbers), std::end(ds::test::sorted_numbers));

	EXPECT_EQ(expected, sorted_by<TypeParam>(values));
}

TYPED_TEST(sorting_algorithm, sorts_a_large_input_with_many_duplicates)
{
	const std::vector<int> values = large_input();

	EXPECT_EQ(expected_for(values), sorted_by<TypeParam>(values));
}

TYPED_TEST(sorting_algorithm, follows_the_comparator)
{
	std::vector<int> values{ 1, 3, 2 };
	TypeParam::apply(values, values.size(), std::greater<int>());

	EXPECT_EQ((std::vector<int>{ 3, 2, 1 }), values);
}

TYPED_TEST(stable_sorting_algorithm, keeps_equal_elements_in_their_original_order)
{
	std::vector<compound_type> values = stability_input();
	TypeParam::apply(values, values.size(), ds::test::by_flag());

	for (std::size_t index = 1; index < values.size(); ++index)
	{
		if (values[index - 1].get_flag() != values[index].get_flag())
			continue;
		EXPECT_LT(values[index - 1].get_number(), values[index].get_number());
	}
}

TYPED_TEST(quick_sort_pivot, sorts_every_input_shape)
{
	std::vector<int> ordered{ -3, -1, 0, 2, 5, 8 };
	std::vector<int> reversed{ 8, 5, 2, 0, -1, -3 };
	std::vector<int> large = large_input();
	const std::vector<int> expected_large = expected_for(large);
	ds::quick_sort::apply<TypeParam>(ordered, ordered.size(), std::less<int>());
	ds::quick_sort::apply<TypeParam>(reversed, reversed.size(), std::less<int>());
	ds::quick_sort::apply<TypeParam>(large, large.size(), std::less<int>());

	EXPECT_EQ(expected_for(ordered), ordered);
	EXPECT_EQ(expected_for(reversed), reversed);
	EXPECT_EQ(expected_large, large);
}

TYPED_TEST(quick_sort_pivot, returns_an_index_inside_the_bounds)
{
	const std::vector<int> values{ 5, 1, 9, 3, 7 };
	TypeParam choose_pivot;
	const std::size_t chosen = choose_pivot(values, ds::detail::index_range{ 1, 3 }, std::less<int>());

	EXPECT_GE(chosen, 1u);
	EXPECT_LE(chosen, 3u);
}

TEST(quick_sort_pivot_choice, median_of_three_picks_the_median_value)
{
	const std::vector<int> values{ 9, 0, 5, 0, 1 };

	EXPECT_EQ(2u, ds::median_of_three()(values, ds::detail::index_range{ 0, 4 }, std::less<int>()));
	EXPECT_EQ(3u, ds::median_of_three()(values, ds::detail::index_range{ 1, 3 }, std::greater<int>()));
}

TEST(quick_sort_pivot_choice, array_sort_forwards_the_pivot_policy)
{
	ds::array<int> values(ds::test::numbers, ds::test::number_count);
	values.sort<ds::quick_sort, ds::middle>();

	EXPECT_EQ(ds::array<int>(ds::test::sorted_numbers, ds::test::number_count), values);
}
