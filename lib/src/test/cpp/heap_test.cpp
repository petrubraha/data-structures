#include <gtest/gtest.h>
#include <algorithm>
#include <functional>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>
#include "heap.hpp"
#include "support/compound_type.hpp"
#include "support/test_values.hpp"

namespace
{
	using ds::test::compound_type;

	// Builds the same heap for both order tags and knows which order the elements leave in.
	template <class order>
	class heap_fixture : public testing::Test
	{
	protected:
		static constexpr bool keeps_greatest_on_top = std::is_same_v<order, ds::max>;
		static constexpr std::size_t npos = ds::heap<order, int>::npos;

		ds::heap<order, int> numbers{ ds::test::numbers, ds::test::number_count };
		ds::heap<order, compound_type, ds::test::by_number> compounds{ ds::test::compounds, ds::test::compound_count };

		// The values in the order extract must return them.
		static std::vector<int> extraction_order(std::vector<int> values)
		{
			if (keeps_greatest_on_top)
				std::sort(values.begin(), values.end(), std::greater<int>());
			else
				std::sort(values.begin(), values.end());
			return values;
		}

		static std::vector<int> shared_extraction_order()
		{
			return extraction_order(std::vector<int>(std::begin(ds::test::numbers), std::end(ds::test::numbers)));
		}

		template <class heap_type>
		static std::vector<int> drain(heap_type& elements)
		{
			std::vector<int> drained;
			while (!elements.empty())
				drained.push_back(elements.extract());
			return drained;
		}
	};

	using orders = testing::Types<ds::max, ds::min>;

	std::string printed(const ds::heap<ds::max, int>& elements)
	{
		std::ostringstream out;
		out << elements;
		return out.str();
	}
}

TYPED_TEST_SUITE(heap_fixture, orders);

//------------------------------------------------
// Constructors and destructor.

TYPED_TEST(heap_fixture, default_constructor_creates_an_empty_heap)
{
	const ds::heap<TypeParam, int> elements;

	EXPECT_TRUE(elements.empty());
	EXPECT_EQ(0u, elements.size());
}

TYPED_TEST(heap_fixture, initializer_list_constructor_puts_the_extreme_on_top)
{
	const ds::heap<TypeParam, int> elements{ 4, 9, 1, 7 };

	EXPECT_EQ(this->keeps_greatest_on_top ? 9 : 1, elements.peek());
}

TYPED_TEST(heap_fixture, raw_buffer_constructor_builds_a_valid_heap)
{
	EXPECT_EQ(ds::test::number_count, this->numbers.size());
	EXPECT_EQ(this->shared_extraction_order(), this->drain(this->numbers));
}

TYPED_TEST(heap_fixture, copy_makes_an_independent_heap)
{
	auto copy = this->compounds;
	copy.extract();

	EXPECT_EQ(ds::test::compound_count, this->compounds.size());
	EXPECT_EQ(ds::test::compound_count - 1, copy.size());
}

TYPED_TEST(heap_fixture, move_steals_the_elements)
{
	auto moved = std::move(this->compounds);

	EXPECT_EQ(ds::test::compound_count, moved.size());
	EXPECT_TRUE(this->compounds.empty());
}

//------------------------------------------------
// Modifier methods.

TYPED_TEST(heap_fixture, copy_assignment_replaces_the_contents)
{
	ds::heap<TypeParam, int> target{ 1 };
	target = this->numbers;

	EXPECT_TRUE(target == this->numbers);
}

TYPED_TEST(heap_fixture, clear_removes_every_element)
{
	this->compounds.clear();

	EXPECT_TRUE(this->compounds.empty());
}

TYPED_TEST(heap_fixture, set_compare_rebuilds_the_heap_for_the_new_order)
{
	ds::heap<TypeParam, int, std::function<bool(const int&, const int&)>> elements;
	elements.set_compare(std::less<int>()).insert(4).insert(9).insert(1).insert(7);
	elements.set_compare(std::greater<int>());

	EXPECT_EQ(this->keeps_greatest_on_top ? 1 : 9, elements.peek());
}

//------------------------------------------------
// Specific methods.

TYPED_TEST(heap_fixture, insert_keeps_the_heap_property)
{
	ds::heap<TypeParam, int> elements;
	elements.insert(5).insert(-2).insert(8).insert(8).insert(0);

	EXPECT_EQ(this->extraction_order({ 5, -2, 8, 8, 0 }), this->drain(elements));
}

TYPED_TEST(heap_fixture, extract_returns_the_top_and_restores_the_heap)
{
	const int top = this->numbers.peek();

	EXPECT_EQ(top, this->numbers.extract());
	EXPECT_EQ(ds::test::number_count - 1, this->numbers.size());
	EXPECT_EQ(this->shared_extraction_order()[1], this->numbers.peek());
}

TYPED_TEST(heap_fixture, extract_of_an_empty_heap_throws)
{
	ds::heap<TypeParam, int> elements;

	EXPECT_THROW(elements.extract(), ds::empty_error);
}

TYPED_TEST(heap_fixture, replace_moves_the_value_up_or_down)
{
	ds::heap<TypeParam, int> rising{ 10, 20, 30, 40, 50 };
	ds::heap<TypeParam, int> sinking{ 10, 20, 30, 40, 50 };
	const int towards_top = this->keeps_greatest_on_top ? 100 : -100;
	const int towards_bottom = -towards_top;
	rising.replace(rising.size() - 1, towards_top);
	sinking.replace(0, towards_bottom);
	const std::vector<int> drained_rising = this->drain(rising);
	const std::vector<int> drained_sinking = this->drain(sinking);

	EXPECT_EQ(towards_top, drained_rising.front());
	EXPECT_EQ(this->extraction_order(drained_rising), drained_rising);
	EXPECT_EQ(towards_bottom, drained_sinking.back());
	EXPECT_EQ(this->extraction_order(drained_sinking), drained_sinking);
}

TYPED_TEST(heap_fixture, replace_with_a_bad_index_throws)
{
	EXPECT_THROW(this->numbers.replace(this->numbers.size(), 1), ds::index_error);
}

//------------------------------------------------
// Constant methods.

TYPED_TEST(heap_fixture, equality_compares_the_stored_layout)
{
	auto same = this->numbers;
	auto different = this->numbers;
	different.extract();

	EXPECT_TRUE(this->numbers == same);
	EXPECT_TRUE(this->numbers != different);
}

TYPED_TEST(heap_fixture, print_writes_the_elements_and_a_newline)
{
	const ds::heap<TypeParam, int> elements{ 3 };
	testing::internal::CaptureStdout();
	elements.print();

	EXPECT_EQ("3\n", testing::internal::GetCapturedStdout());
}

TYPED_TEST(heap_fixture, peek_of_an_empty_heap_throws)
{
	const ds::heap<TypeParam, int> elements;

	EXPECT_THROW(elements.peek(), ds::empty_error);
}

TYPED_TEST(heap_fixture, peek_respects_a_custom_comparator)
{
	EXPECT_EQ(this->keeps_greatest_on_top ? 20 : 12, this->compounds.peek().get_number());
}

//------------------------------------------------
// Query operations.

TYPED_TEST(heap_fixture, search_returns_the_index_of_a_stored_value)
{
	for (const int value : ds::test::numbers)
	{
		const std::size_t index = this->numbers.search(value);
		ASSERT_NE(this->npos, index);
		EXPECT_EQ(value, convert(this->numbers)[index]);
	}
}

TYPED_TEST(heap_fixture, search_returns_npos_for_a_missing_value)
{
	const ds::heap<TypeParam, int> empty_heap;

	EXPECT_EQ(this->npos, this->numbers.search(8));
	EXPECT_EQ(this->npos, empty_heap.search(8));
}

//------------------------------------------------
// Iterator methods.

TYPED_TEST(heap_fixture, iteration_starts_at_the_top_and_visits_every_element)
{
	std::size_t visited = 0;
	for (const int value : this->numbers)
	{
		if (visited == 0)
			EXPECT_EQ(this->numbers.peek(), value);
		++visited;
	}

	EXPECT_EQ(ds::test::number_count, visited);
}

//------------------------------------------------
// Friend functions.

TEST(heap, stream_output_lists_the_levels_in_order)
{
	const ds::heap<ds::max, int> elements{ 1, 2, 3 };

	EXPECT_EQ("3 2 1", printed(elements));
}

TYPED_TEST(heap_fixture, convert_copies_the_level_order)
{
	const auto block = convert(this->compounds);
	std::size_t index = 0;
	for (const compound_type& value : this->compounds)
		EXPECT_EQ(value, block[index++]);

	EXPECT_EQ(ds::test::compound_count, index);
}
