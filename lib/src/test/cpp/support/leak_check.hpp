#pragma once
#include <cstddef>
#include <gtest/gtest.h>

namespace ds::test
{
	// Number of blocks obtained through the global operator new and not yet released.
	std::size_t live_allocations() noexcept;

	// Fails the running test when it ends with more live allocations than it started with.
	// Death test suites, named with a "_death_test" suffix, are skipped.
	class leak_check : public testing::EmptyTestEventListener
	{
	public:
		void OnTestStart(const testing::TestInfo& test) override;
		void OnTestEnd(const testing::TestInfo& test) override;

	private:
		std::size_t allocations_at_start = 0;
	};
}
