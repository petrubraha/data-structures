#include "leak_check.hpp"
#include <atomic>
#include <cstdlib>
#include <new>
#include <string_view>

namespace
{
	// Death test suites end with this suffix; gtest keeps per-process bookkeeping for them in the parent.
	constexpr std::string_view death_test_suffix = "_death_test";

	std::atomic<std::size_t> allocation_count{ 0 };

	bool is_death_test(const testing::TestInfo& test)
	{
		const std::string_view suite = test.test_suite_name();
		return suite.size() >= death_test_suffix.size()
			&& suite.substr(suite.size() - death_test_suffix.size()) == death_test_suffix;
	}

	void* allocate(std::size_t byte_count)
	{
		void* block = std::malloc(byte_count == 0 ? 1 : byte_count);
		if (block == nullptr)
			throw std::bad_alloc();

		allocation_count.fetch_add(1, std::memory_order_relaxed);
		return block;
	}

	void release(void* block) noexcept
	{
		if (block == nullptr)
			return;

		allocation_count.fetch_sub(1, std::memory_order_relaxed);
		std::free(block);
	}
}

// Replacing the global allocation functions lets every test count the blocks it leaves behind.
void* operator new(std::size_t byte_count)
{
	return allocate(byte_count);
}

void* operator new[](std::size_t byte_count)
{
	return allocate(byte_count);
}

void operator delete(void* block) noexcept
{
	release(block);
}

void operator delete[](void* block) noexcept
{
	release(block);
}

void operator delete(void* block, std::size_t) noexcept
{
	release(block);
}

void operator delete[](void* block, std::size_t) noexcept
{
	release(block);
}

namespace ds::test
{
	std::size_t live_allocations() noexcept
	{
		return allocation_count.load(std::memory_order_relaxed);
	}

	void leak_check::OnTestStart(const testing::TestInfo&)
	{
		allocations_at_start = live_allocations();
	}

	void leak_check::OnTestEnd(const testing::TestInfo& test)
	{
		if (is_death_test(test))
			return;

		const std::size_t allocations_at_end = live_allocations();
		EXPECT_EQ(allocations_at_start, allocations_at_end)
			<< test.test_suite_name() << '.' << test.name() << " leaked "
			<< allocations_at_end - allocations_at_start << " allocation(s)";
	}
}
