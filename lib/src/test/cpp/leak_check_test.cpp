#include <gtest/gtest.h>
#include <memory>
#include "support/leak_check.hpp"

TEST(leak_check, counts_a_live_allocation)
{
	const std::size_t before = ds::test::live_allocations();
	auto block = std::make_unique<int>(7);

	EXPECT_EQ(before + 1, ds::test::live_allocations());
}

TEST(leak_check, releases_the_count_on_delete)
{
	const std::size_t before = ds::test::live_allocations();
	delete new int(7);

	EXPECT_EQ(before, ds::test::live_allocations());
}
