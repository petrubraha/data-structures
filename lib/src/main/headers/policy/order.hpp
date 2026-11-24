#pragma once
#include "utils/linear_queries.hpp"

// Heap order tags. Each turns the heap's comparator into the ordering the sift operations use: an element
// rises above its parent when the parent is ordered before it.
namespace ds
{
	// The greatest element, by the comparator, stays on top.
	struct max
	{
		template <class compare>
		static const compare& sift_order(const compare& order) noexcept
		{
			return order;
		}
	};

	// The least element, by the comparator, stays on top.
	struct min
	{
		template <class compare>
		static detail::reversed<compare> sift_order(const compare& order) noexcept
		{
			return detail::reversed<compare>(order);
		}
	};
}
