#pragma once
#include <cstddef>
#include "positional_access.hpp"

namespace ds::detail
{
	// Unstable. Performs at most one swap per position.
	template <class sequence, class ordering>
	void selection_sort(sequence& values, std::size_t length, const ordering& before)
	{
		for (std::size_t position = 0; position + 1 < length; ++position)
		{
			std::size_t smallest = position;
			for (std::size_t index = position + 1; index < length; ++index)
				if (before(values[index], values[smallest]))
					smallest = index;

			if (smallest != position)
				swap_elements(values, smallest, position);
		}
	}
}

namespace ds
{
	struct selection_sort
	{
		template <class sequence, class ordering>
		static void apply(sequence& values, std::size_t length, const ordering& before)
		{
			detail::selection_sort(values, length, before);
		}
	};
}
