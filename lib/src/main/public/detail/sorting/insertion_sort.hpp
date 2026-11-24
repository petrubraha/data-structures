#pragma once
#include <cstddef>
#include <utility>
#include "positional_access.hpp"

namespace ds::detail
{
	// Stable. Shifts larger elements right instead of swapping, one move per step.
	template <class sequence, class ordering>
	void insertion_sort(sequence& values, std::size_t length, const ordering& before)
	{
		for (std::size_t index = 1; index < length; ++index)
		{
			element_of<sequence> key = std::move(values[index]);
			std::size_t hole = index;
			for (; hole > 0 && before(key, values[hole - 1]); --hole)
				values[hole] = std::move(values[hole - 1]);
			values[hole] = std::move(key);
		}
	}
}

namespace ds
{
	struct insertion_sort
	{
		template <class sequence, class ordering>
		static void apply(sequence& values, std::size_t length, const ordering& before)
		{
			detail::insertion_sort(values, length, before);
		}
	};
}
