#pragma once
#include <cstddef>
#include "positional_access.hpp"

namespace ds::detail
{
	// Stable. Each pass stops at the last swap of the previous one, so sorted tails are skipped.
	template <class sequence, class ordering>
	void bubble_sort(sequence& values, std::size_t length, const ordering& before)
	{
		std::size_t unsorted = length;
		while (unsorted > 1)
		{
			std::size_t last_swap = 0;
			for (std::size_t index = 1; index < unsorted; ++index)
			{
				if (!before(values[index], values[index - 1]))
					continue;
				swap_elements(values, index - 1, index);
				last_swap = index;
			}

			unsorted = last_swap;
		}
	}
}

namespace ds
{
	struct bubble_sort
	{
		template <class sequence, class ordering>
		static void apply(sequence& values, std::size_t length, const ordering& before)
		{
			detail::bubble_sort(values, length, before);
		}
	};
}
