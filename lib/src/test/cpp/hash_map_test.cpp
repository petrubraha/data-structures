#include <gtest/gtest.h>
#include <cstddef>
#include <sstream>
#include <string>
#include <utility>
#include "hash_map.hpp"
#include "support/compound_type.hpp"
#include "support/test_values.hpp"
#include "support/throwing_copy.hpp"

namespace
{
	using ds::test::compound_type;
	using numbers_type = ds::hash_map<int, std::string>;
	using compounds_type = ds::hash_map<compound_type, int, ds::test::compound_hash>;

	// Sends every key to the same bucket, so every operation runs through collisions.
	struct colliding_hash
	{
		std::size_t operator()(int) const noexcept
		{
			return 0;
		}
	};

	// Spreads the shared numbers so the map grows past its default bucket count.
	constexpr int growth_key_count = 200;

	class hash_map_fixture : public testing::Test
	{
	protected:
		numbers_type numbers;
		compounds_type compounds;

		hash_map_fixture()
		{
			for (const int number : ds::test::numbers)
				numbers.insert(number, std::to_string(number));
			for (std::size_t index = 0; index < ds::test::compound_count; ++index)
				compounds.insert(ds::test::compounds[index], static_cast<int>(index));
		}
	};

	template <class map_type>
	std::size_t visited_count(const map_type& map)
	{
		std::size_t visited = 0;
		for (const auto& entry : map)
			visited += map.contains(entry.first) ? 1 : 0;
		return visited;
	}
}

//------------------------------------------------
// Constructors and destructor.

TEST(hash_map, default_constructor_creates_an_empty_map_without_buckets)
{
	const numbers_type map;

	EXPECT_TRUE(map.empty());
	EXPECT_FALSE(map.contains(1));
	EXPECT_FALSE(map.begin() != map.end());
}

TEST(hash_map, initializer_list_constructor_keeps_the_last_value_of_a_repeated_key)
{
	const numbers_type map{ { 1, "one" }, { 2, "two" }, { 1, "uno" } };

	EXPECT_EQ(2u, map.size());
	EXPECT_EQ("uno", map.get(1));
}

TEST(hash_map, raw_buffer_constructor_inserts_every_pair)
{
	const numbers_type::value_type pairs[] = { { 3, "three" }, { 4, "four" } };
	const numbers_type map(pairs, std::size(pairs));

	EXPECT_EQ("three", map.get(3));
	EXPECT_EQ("four", map.get(4));
}

TEST_F(hash_map_fixture, copy_constructor_makes_an_independent_copy)
{
	compounds_type copy(compounds);
	copy.remove(ds::test::compounds[0]);

	EXPECT_EQ(ds::test::compound_count, compounds.size());
	EXPECT_FALSE(copy.contains(ds::test::compounds[0]));
	EXPECT_TRUE(copy.contains(ds::test::compounds[1]));
}

TEST(hash_map, copy_constructor_releases_everything_when_a_copy_throws)
{
	ds::hash_map<int, ds::test::throwing_copy> source;
	source.insert(1, ds::test::throwing_copy(1)).insert(2, ds::test::throwing_copy(2));
	const ds::test::copy_budget budget(1);

	EXPECT_THROW((ds::hash_map<int, ds::test::throwing_copy>{ source }), std::runtime_error);
}

TEST_F(hash_map_fixture, move_constructor_steals_the_buckets)
{
	const compounds_type moved(std::move(compounds));

	EXPECT_EQ(ds::test::compound_count, moved.size());
	EXPECT_TRUE(compounds.empty());
	EXPECT_FALSE(compounds.contains(ds::test::compounds[0]));
}

//------------------------------------------------
// Modifier methods.

TEST_F(hash_map_fixture, copy_assignment_replaces_the_contents)
{
	numbers_type target{ { 100, "hundred" } };
	target = numbers;

	EXPECT_TRUE(target == numbers);
	EXPECT_FALSE(target.contains(100));
}

TEST_F(hash_map_fixture, move_assignment_steals_the_buckets)
{
	compounds_type target;
	target.insert(ds::test::compounds[0], 99);
	target = std::move(compounds);

	EXPECT_EQ(ds::test::compound_count, target.size());
	EXPECT_EQ(0, target.get(ds::test::compounds[0]));
	EXPECT_TRUE(compounds.empty());
}

TEST_F(hash_map_fixture, move_assignment_to_itself_keeps_the_contents)
{
	numbers_type& same = numbers;
	numbers = std::move(same);

	EXPECT_EQ("255", numbers.get(255));
}

TEST_F(hash_map_fixture, clear_removes_every_pair_and_allows_reuse)
{
	numbers.clear();
	numbers.insert(5, "five");

	EXPECT_EQ(1u, numbers.size());
	EXPECT_FALSE(numbers.contains(255));
}

//------------------------------------------------
// Specific methods.

TEST_F(hash_map_fixture, insert_stores_each_distinct_key_once)
{
	EXPECT_EQ(ds::test::number_count - 1, numbers.size());
	EXPECT_EQ("-2147483648", numbers.get(INT_MIN));
}

TEST_F(hash_map_fixture, insert_of_a_present_key_replaces_its_value)
{
	numbers.insert(7, "seven");

	EXPECT_EQ("seven", numbers.get(7));
	EXPECT_EQ(ds::test::number_count - 1, numbers.size());
}

TEST(hash_map, insert_grows_the_table_and_keeps_every_pair)
{
	ds::hash_map<int, int> map;
	for (int key = 0; key < growth_key_count; ++key)
		map.insert(key, key * key);

	EXPECT_EQ(static_cast<std::size_t>(growth_key_count), map.size());
	for (int key = 0; key < growth_key_count; ++key)
		EXPECT_EQ(key * key, map.get(key));
}

TEST_F(hash_map_fixture, remove_deletes_only_the_given_key)
{
	numbers.remove(7).remove(INT_MAX);

	EXPECT_FALSE(numbers.contains(7));
	EXPECT_FALSE(numbers.contains(INT_MAX));
	EXPECT_TRUE(numbers.contains(0));
	EXPECT_EQ(ds::test::number_count - 3, numbers.size());
}

TEST_F(hash_map_fixture, remove_of_a_missing_key_changes_nothing)
{
	numbers.remove(8);
	numbers_type().remove(8);

	EXPECT_EQ(ds::test::number_count - 1, numbers.size());
}

TEST(hash_map, colliding_keys_share_a_bucket_and_stay_distinct)
{
	ds::hash_map<int, int, colliding_hash> map;
	map.insert(1, 10).insert(2, 20).insert(3, 30).remove(2).insert(1, 11);

	EXPECT_EQ(2u, map.size());
	EXPECT_EQ(11, map.get(1));
	EXPECT_FALSE(map.contains(2));
	EXPECT_EQ(30, map.get(3));
}

//------------------------------------------------
// Constant methods.

TEST_F(hash_map_fixture, equality_ignores_insertion_order)
{
	numbers_type reversed;
	for (std::size_t index = ds::test::number_count; index > 0; --index)
		reversed.insert(ds::test::numbers[index - 1], std::to_string(ds::test::numbers[index - 1]));
	numbers_type different(numbers);
	different.insert(0, "zero");

	EXPECT_TRUE(numbers == reversed);
	EXPECT_TRUE(numbers != different);
	EXPECT_TRUE(numbers != (numbers_type{ { 1, "1" } }));
}

TEST(hash_map, print_writes_the_pairs_and_a_newline)
{
	const numbers_type map{ { 1, "one" } };
	testing::internal::CaptureStdout();
	map.print();

	EXPECT_EQ("1: one\n", testing::internal::GetCapturedStdout());
}

TEST_F(hash_map_fixture, get_returns_a_writable_value)
{
	numbers.get(0) = "nil";
	const numbers_type& view = numbers;

	EXPECT_EQ("nil", view.get(0));
}

TEST_F(hash_map_fixture, get_of_a_missing_key_throws)
{
	const numbers_type& view = numbers;

	EXPECT_THROW(numbers.get(8), ds::key_error);
	EXPECT_THROW(view.get(8), ds::key_error);
}

TEST_F(hash_map_fixture, contains_reports_presence_by_key)
{
	EXPECT_TRUE(compounds.contains(ds::test::compounds[2]));
	EXPECT_FALSE(compounds.contains(compound_type(2, false, "absent")));
}

//------------------------------------------------
// Iterator methods.

TEST_F(hash_map_fixture, iteration_visits_every_pair_once)
{
	EXPECT_EQ(numbers.size(), visited_count(numbers));
	EXPECT_EQ(compounds.size(), visited_count(compounds));
}

TEST_F(hash_map_fixture, mutable_iteration_writes_the_values)
{
	for (auto& entry : numbers)
		entry.second = "same";

	EXPECT_EQ("same", numbers.get(INT_MIN));
	EXPECT_EQ("same", numbers.get(65535));
}

//------------------------------------------------
// Friend functions.

TEST(hash_map, stream_output_writes_key_value_pairs)
{
	std::ostringstream out;
	out << ds::hash_map<int, int, colliding_hash>{ { 1, 10 }, { 2, 20 } } << '|' << numbers_type();

	EXPECT_EQ("2: 20 1: 10|", out.str());
}

TEST_F(hash_map_fixture, convert_copies_every_pair)
{
	const auto block = convert(numbers);

	for (std::size_t index = 0; index < numbers.size(); ++index)
		EXPECT_EQ(std::to_string(block[index].first), numbers.get(block[index].first));
}
