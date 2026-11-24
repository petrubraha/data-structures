#pragma once
#include <cstddef>
#include "pivot.hpp"
#include "positional_access.hpp"

namespace ds::detail
{
	template <class sequence, class ordering, class pivot>
	struct partition_context
	{
		sequence& values;
		const ordering& before;
		pivot& choose_pivot;
	};

	// Hoare partition around the chosen pivot, first moved to the front so the split never lands on the
	// last index. Returns the last index of the left part.
	template <class context_type>
	std::size_t partition(context_type& context, index_range bounds)
	{
		const std::size_t pivot_index = context.choose_pivot(context.values, bounds, context.before);
		swap_elements(context.values, bounds.first, pivot_index);
		const auto pivot_value = context.values[bounds.first];

		std::size_t low = bounds.first;
		std::size_t high = bounds.last;
		while (true)
		{
			while (context.before(context.values[low], pivot_value))
				++low;
			while (context.before(pivot_value, context.values[high]))
				--high;
			if (low >= high)
				return high;

			swap_elements(context.values, low, high);
			++low;
			--high;
		}
	}

	// Recurses into the smaller part and loops over the larger, so the stack depth stays logarithmic.
	template <class context_type>
	void quick_sort_range(context_type& context, index_range bounds)
	{
		while (bounds.first < bounds.last)
		{
			const std::size_t split = partition(context, bounds);
			if (split - bounds.first < bounds.last - split)
			{
				quick_sort_range(context, { bounds.first, split });
				bounds.first = split + 1;
			}
			else
			{
				quick_sort_range(context, { split + 1, bounds.last });
				bounds.last = split;
			}
		}
	}

	// Unstable. The pivot policy picks the partition element; the partition itself is written once.
	template <class pivot = median_of_three, class sequence, class ordering>
	void quick_sort(sequence& values, std::size_t length, const ordering& before)
	{
		if (length < 2)
			return;

		pivot choose_pivot;
		partition_context<sequence, ordering, pivot> context{ values, before, choose_pivot };
		quick_sort_range(context, { 0, length - 1 });
	}
}

namespace ds
{
	struct quick_sort
	{
		template <class pivot = median_of_three, class sequence, class ordering>
		static void apply(sequence& values, std::size_t length, const ordering& before)
		{
			detail::quick_sort<pivot>(values, length, before);
		}
	};
}
