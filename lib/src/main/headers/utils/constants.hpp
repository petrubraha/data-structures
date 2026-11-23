#pragma once
#include <cstddef>

namespace ds::detail
{
	// Returned by index-based queries when no element matches.
	inline constexpr std::size_t npos = static_cast<std::size_t>(-1);

	// Elements reserved by an array-backed structure built without an explicit capacity.
	inline constexpr std::size_t default_capacity = 16;

	// Multiplier applied to the capacity when an array-backed structure runs out of room.
	inline constexpr std::size_t growth_factor = 2;

	// Buckets allocated by an empty hash map.
	inline constexpr std::size_t default_bucket_count = 16;

	// A hash map rehashes once its element count exceeds this many elements per bucket.
	inline constexpr std::size_t maximum_load_factor = 1;
}
