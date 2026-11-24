#include <gtest/gtest.h>
#include <string>
#include "detail/utils/exceptions.hpp"

TEST(exceptions, index_error_names_operation_index_and_size)
{
	const ds::index_error error("array::get", 7, 5);

	EXPECT_STREQ("array::get: index 7 out of range, size is 5", error.what());
}

TEST(exceptions, empty_error_names_operation)
{
	const ds::empty_error error("stack::top");

	EXPECT_STREQ("stack::top: no element to access, structure is empty", error.what());
}

TEST(exceptions, key_error_names_operation)
{
	const ds::key_error error("hash_map::get");

	EXPECT_STREQ("hash_map::get: key not found", error.what());
}

TEST(exceptions, position_error_names_operation_and_reason)
{
	const ds::position_error error("linked_list::insert_after", "position is the end");

	EXPECT_STREQ("linked_list::insert_after: position is the end", error.what());
}

TEST(exceptions, capacity_error_names_requested_and_maximum)
{
	const ds::capacity_error error("array::array", 9, 4);

	EXPECT_STREQ("array::array: capacity 9 exceeds the maximum of 4", error.what());
}

TEST(exceptions, errors_derive_from_the_standard_hierarchy)
{
	EXPECT_THROW(throw ds::index_error("array::get", 0, 0), std::out_of_range);
	EXPECT_THROW(throw ds::empty_error("stack::top"), std::out_of_range);
	EXPECT_THROW(throw ds::key_error("hash_map::get"), std::out_of_range);
	EXPECT_THROW(throw ds::position_error("linked_list::remove_after", "no element after the position"), std::out_of_range);
	EXPECT_THROW(throw ds::capacity_error("array::array", 1, 0), std::length_error);
}

TEST(exceptions_death_test, fail_fast_terminates_with_the_message)
{
	EXPECT_DEATH(ds::detail::fail_fast("tree_iterator: advanced past the end"), "advanced past the end");
}
