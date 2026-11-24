#include <gtest/gtest.h>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <random>
#include <sstream>
#include <utility>
#include <vector>
#include "support/compound_type.hpp"
#include "support/test_values.hpp"
#include "support/throwing_copy.hpp"
#include "tree.hpp"

namespace ds::detail
{
	struct tree_inspector
	{
		template <class tree_type>
		static auto root(const tree_type& values)
		{
			return values.root;
		}
	};
}

namespace
{
	using ds::detail::side;
	using ds::test::compound_type;

	constexpr std::size_t stress_length = 300;
	constexpr unsigned stress_seed = 20240607;

	template <class node_type>
	bool links_are_consistent(const node_type* subtree)
	{
		if (subtree == nullptr)
			return true;

		for (const side child_side : { side::left, side::right })
		{
			const node_type* child = subtree->child(child_side);
			if (child != nullptr && (child->parent != subtree || !links_are_consistent(child)))
				return false;
		}

		return true;
	}

	// The subtree height, or -1 when an AVL invariant fails below.
	template <class node_type>
	int avl_height(const node_type* subtree)
	{
		if (subtree == nullptr)
			return 0;

		const int left = avl_height(subtree->child(side::left));
		const int right = avl_height(subtree->child(side::right));
		if (left < 0 || right < 0 || std::abs(left - right) > 1)
			return -1;

		const int height = 1 + std::max(left, right);
		return height == subtree->augment ? height : -1;
	}

	template <class node_type>
	bool is_red(const node_type* subtree)
	{
		return subtree != nullptr && subtree->augment == ds::red_black::colour::red;
	}

	// The number of black nodes on every path down, or -1 when a red-black invariant fails below.
	template <class node_type>
	int black_height(const node_type* subtree)
	{
		if (subtree == nullptr)
			return 1;
		if (is_red(subtree) && (is_red(subtree->child(side::left)) || is_red(subtree->child(side::right))))
			return -1;

		const int left = black_height(subtree->child(side::left));
		const int right = black_height(subtree->child(side::right));
		if (left < 0 || left != right)
			return -1;
		return left + (is_red(subtree) ? 0 : 1);
	}

	template <class tree_type>
	bool is_balanced(const tree_type& values, ds::avl)
	{
		return avl_height(ds::detail::tree_inspector::root(values)) >= 0;
	}

	template <class tree_type>
	bool is_balanced(const tree_type& values, ds::red_black)
	{
		const auto root = ds::detail::tree_inspector::root(values);
		return !is_red(root) && black_height(root) > 0;
	}

	// Parent links, strict in-order ordering, the element count and the policy's balance all hold.
	template <class balancing, class tree_type, class ordering = std::less<>>
	testing::AssertionResult satisfies_invariants(const tree_type& values, const ordering& before = ordering())
	{
		const auto root = ds::detail::tree_inspector::root(values);
		if (root != nullptr && root->parent != nullptr)
			return testing::AssertionFailure() << "the root has a parent";
		if (!links_are_consistent(root))
			return testing::AssertionFailure() << "a child does not point back to its parent";

		std::vector<typename tree_type::value_type> visited;
		for (const auto& value : values)
			visited.push_back(value);
		if (visited.size() != values.size())
			return testing::AssertionFailure() << "traversal count differs from size";
		const auto out_of_order = [&before](const auto& left, const auto& right) { return !before(left, right); };
		if (std::adjacent_find(visited.begin(), visited.end(), out_of_order) != visited.end())
			return testing::AssertionFailure() << "traversal is not strictly increasing";
		if (!is_balanced(values, balancing()))
			return testing::AssertionFailure() << "the balancing invariant is broken";
		return testing::AssertionSuccess();
	}

	template <class balancing>
	class tree_fixture : public testing::Test
	{
	protected:
		using numbers_type = ds::tree<balancing, int>;
		using compounds_type = ds::tree<balancing, compound_type, ds::test::by_text>;

		numbers_type numbers{ ds::test::numbers, ds::test::number_count };
		compounds_type compounds{ ds::test::compounds, ds::test::compound_count };

		static std::vector<int> sorted_unique_numbers()
		{
			std::vector<int> values(std::begin(ds::test::sorted_numbers), std::end(ds::test::sorted_numbers));
			values.erase(std::unique(values.begin(), values.end()), values.end());
			return values;
		}

		template <class tree_type>
		static std::vector<int> contents(const tree_type& values)
		{
			std::vector<int> visited;
			for (const int value : values)
				visited.push_back(value);
			return visited;
		}

		static std::vector<int> shuffled_range()
		{
			std::vector<int> values(stress_length);
			for (std::size_t index = 0; index < stress_length; ++index)
				values[index] = static_cast<int>(index);
			std::shuffle(values.begin(), values.end(), std::mt19937(stress_seed));
			return values;
		}
	};

	using balancings = testing::Types<ds::avl, ds::red_black>;

	std::string printed(const ds::tree<ds::avl, int>& values)
	{
		std::ostringstream out;
		out << values;
		return out.str();
	}
}

TYPED_TEST_SUITE(tree_fixture, balancings);

//------------------------------------------------
// Constructors and destructor.

TYPED_TEST(tree_fixture, default_constructor_creates_an_empty_tree)
{
	const typename TestFixture::numbers_type values;

	EXPECT_TRUE(values.empty());
	EXPECT_FALSE(values.begin() != values.end());
}

TYPED_TEST(tree_fixture, initializer_list_constructor_sorts_and_drops_duplicates)
{
	const typename TestFixture::numbers_type values{ 3, 1, 3, 2 };

	EXPECT_EQ((std::vector<int>{ 1, 2, 3 }), this->contents(values));
	EXPECT_TRUE(satisfies_invariants<TypeParam>(values));
}

TYPED_TEST(tree_fixture, raw_buffer_constructor_inserts_every_distinct_value)
{
	EXPECT_EQ(this->sorted_unique_numbers(), this->contents(this->numbers));
	EXPECT_TRUE(satisfies_invariants<TypeParam>(this->numbers));
}

TYPED_TEST(tree_fixture, copy_constructor_clones_an_independent_valid_tree)
{
	typename TestFixture::compounds_type copy(this->compounds);
	copy.remove(ds::test::compounds[0]);

	EXPECT_EQ(ds::test::compound_count, this->compounds.size());
	EXPECT_EQ(ds::test::compound_count - 1, copy.size());
	EXPECT_TRUE(satisfies_invariants<TypeParam>(copy, ds::test::by_text()));
}

TYPED_TEST(tree_fixture, copy_constructor_releases_the_nodes_when_an_element_copy_throws)
{
	const ds::tree<TypeParam, ds::test::throwing_copy> source{ ds::test::throwing_copy(1), ds::test::throwing_copy(2) };
	const ds::test::copy_budget budget(1);

	EXPECT_THROW((ds::tree<TypeParam, ds::test::throwing_copy>{ source }), std::runtime_error);
}

TYPED_TEST(tree_fixture, move_constructor_steals_the_nodes)
{
	const auto moved = std::move(this->compounds);

	EXPECT_EQ(ds::test::compound_count, moved.size());
	EXPECT_TRUE(this->compounds.empty());
}

//------------------------------------------------
// Modifier methods.

TYPED_TEST(tree_fixture, copy_assignment_replaces_the_contents)
{
	typename TestFixture::numbers_type target{ 1, 2 };
	target = this->numbers;

	EXPECT_TRUE(target == this->numbers);
	EXPECT_TRUE(satisfies_invariants<TypeParam>(target));
}

TYPED_TEST(tree_fixture, move_assignment_steals_the_nodes)
{
	typename TestFixture::compounds_type target{ ds::test::compounds[0] };
	target = std::move(this->compounds);

	EXPECT_EQ(ds::test::compound_count, target.size());
	EXPECT_TRUE(this->compounds.empty());
}

TYPED_TEST(tree_fixture, clear_removes_every_node)
{
	this->compounds.clear();

	EXPECT_TRUE(this->compounds.empty());
	EXPECT_FALSE(this->compounds.begin() != this->compounds.end());
}

TYPED_TEST(tree_fixture, set_compare_relinks_the_nodes_in_the_new_order)
{
	ds::tree<TypeParam, int, std::function<bool(const int&, const int&)>> values;
	values.set_compare(std::less<int>()).insert(2).insert(3).insert(1);
	values.set_compare(std::greater<int>());

	EXPECT_EQ((std::vector<int>{ 3, 2, 1 }), this->contents(values));
	EXPECT_TRUE(satisfies_invariants<TypeParam>(values, std::greater<>()));
}

TYPED_TEST(tree_fixture, set_compare_drops_values_that_become_equivalent)
{
	ds::tree<TypeParam, int, std::function<bool(const int&, const int&)>> values;
	values.set_compare(std::less<int>()).insert(1).insert(2).insert(3).insert(4);
	values.set_compare([](const int& left, const int& right) { return left % 2 < right % 2; });

	EXPECT_EQ(2u, values.size());
}

//------------------------------------------------
// Specific methods.

TYPED_TEST(tree_fixture, insert_ignores_a_value_already_present)
{
	this->numbers.insert(7).insert(INT_MAX);

	EXPECT_EQ(this->sorted_unique_numbers().size(), this->numbers.size());
}

TYPED_TEST(tree_fixture, insert_keeps_the_tree_balanced)
{
	typename TestFixture::numbers_type values;
	for (const int value : this->shuffled_range())
	{
		values.insert(value);
		ASSERT_TRUE(satisfies_invariants<TypeParam>(values));
	}

	EXPECT_EQ(stress_length, values.size());
}

TYPED_TEST(tree_fixture, ascending_inserts_stay_balanced)
{
	typename TestFixture::numbers_type values;
	for (std::size_t value = 0; value < stress_length; ++value)
		values.insert(static_cast<int>(value));

	EXPECT_TRUE(satisfies_invariants<TypeParam>(values));
}

TYPED_TEST(tree_fixture, remove_handles_leaves_single_children_and_two_children)
{
	typename TestFixture::numbers_type values{ 50, 30, 70, 20, 40, 60, 80, 10 };
	values.remove(10).remove(20).remove(30).remove(50);

	EXPECT_EQ((std::vector<int>{ 40, 60, 70, 80 }), this->contents(values));
	EXPECT_TRUE(satisfies_invariants<TypeParam>(values));
}

TYPED_TEST(tree_fixture, remove_of_a_missing_value_changes_nothing)
{
	this->numbers.remove(8);

	EXPECT_EQ(this->sorted_unique_numbers(), this->contents(this->numbers));
}

TYPED_TEST(tree_fixture, remove_keeps_the_tree_balanced)
{
	typename TestFixture::numbers_type values;
	for (const int value : this->shuffled_range())
		values.insert(value);

	std::vector<int> removal_order = this->shuffled_range();
	std::reverse(removal_order.begin(), removal_order.end());
	for (const int value : removal_order)
	{
		values.remove(value);
		ASSERT_TRUE(satisfies_invariants<TypeParam>(values));
	}

	EXPECT_TRUE(values.empty());
}

//------------------------------------------------
// Constant methods.

TYPED_TEST(tree_fixture, equality_compares_the_values_in_order)
{
	typename TestFixture::numbers_type same(ds::test::sorted_numbers, ds::test::number_count);
	typename TestFixture::numbers_type different(this->numbers);
	different.remove(0);

	EXPECT_TRUE(this->numbers == same);
	EXPECT_TRUE(this->numbers != different);
	EXPECT_TRUE(this->numbers != (typename TestFixture::numbers_type{ 1 }));
}

TYPED_TEST(tree_fixture, print_writes_the_values_and_a_newline)
{
	const typename TestFixture::numbers_type values{ 3, -2, 1 };
	testing::internal::CaptureStdout();
	values.print();

	EXPECT_EQ("-2 1 3\n", testing::internal::GetCapturedStdout());
}

//------------------------------------------------
// Query operations.

TYPED_TEST(tree_fixture, search_finds_a_stored_value_or_returns_the_end)
{
	EXPECT_EQ(255, *this->numbers.search(255));
	EXPECT_FALSE(this->numbers.search(8) != this->numbers.end());
}

TYPED_TEST(tree_fixture, minimum_and_maximum_return_the_extreme_values)
{
	EXPECT_EQ(INT_MIN, *this->numbers.minimum());
	EXPECT_EQ(INT_MAX, *this->numbers.maximum());
	EXPECT_EQ(ds::test::compounds[1], *this->compounds.minimum());
	EXPECT_EQ(ds::test::compounds[4], *this->compounds.maximum());
}

TYPED_TEST(tree_fixture, minimum_and_maximum_of_an_empty_tree_throw)
{
	const typename TestFixture::numbers_type values;

	EXPECT_THROW(values.minimum(), ds::empty_error);
	EXPECT_THROW(values.maximum(), ds::empty_error);
}

TYPED_TEST(tree_fixture, predecessor_and_successor_walk_in_order)
{
	const auto seven = this->numbers.search(7);

	EXPECT_EQ(3, *this->numbers.predecessor(seven));
	EXPECT_EQ(255, *this->numbers.successor(seven));
	EXPECT_FALSE(this->numbers.predecessor(this->numbers.minimum()) != this->numbers.end());
	EXPECT_FALSE(this->numbers.successor(this->numbers.maximum()) != this->numbers.end());
}

TYPED_TEST(tree_fixture, predecessor_and_successor_of_the_end_throw)
{
	EXPECT_THROW(this->numbers.predecessor(this->numbers.end()), ds::position_error);
	EXPECT_THROW(this->numbers.successor(this->numbers.end()), ds::position_error);
}

//------------------------------------------------
// Friend functions.

TEST(tree, stream_output_lists_the_values_in_order)
{
	const ds::tree<ds::avl, int> values{ 2, 3, 1 };

	EXPECT_EQ("1 2 3", printed(values));
	EXPECT_EQ("", printed(ds::tree<ds::avl, int>()));
}

TYPED_TEST(tree_fixture, convert_copies_the_values_in_order)
{
	const auto block = convert(this->compounds);
	std::size_t index = 0;
	for (const compound_type& value : this->compounds)
		EXPECT_EQ(value, block[index++]);

	EXPECT_EQ(ds::test::compound_count, index);
}
