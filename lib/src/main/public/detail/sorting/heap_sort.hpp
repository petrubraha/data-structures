#pragma once
#include <cstddef>
#include "positional_access.hpp"

namespace ds::detail
{
	// Restores the heap property below the start, within the first length elements. The element that
	// no child is ordered after ends up on top, so an ascending ordering builds a max-heap.
	template <class sequence, class ordering>
	void sift_down(sequence& values, std::size_t start, std::size_t length, const ordering& before)
	{
		std::size_t parent = start;
		while (true)
		{
			std::size_t largest = parent;
			const std::size_t left = 2 * parent + 1;
			const std::size_t right = left + 1;
			if (left < length && before(values[largest], values[left]))
				largest = left;
			if (right < length && before(values[largest], values[right]))
				largest = right;
			if (largest == parent)
				return;

			swap_elements(values, parent, largest);
			parent = largest;
		}
	}

	// Arranges the first length elements into a heap in linear time.
	template <class sequence, class ordering>
	void build_heap(sequence& values, std::size_t length, const ordering& before)
	{
		for (std::size_t parent = length / 2; parent > 0; --parent)
			sift_down(values, parent - 1, length, before);
	}

	// Unstable. In place, with no extra memory.
	template <class sequence, class ordering>
	void heap_sort(sequence& values, std::size_t length, const ordering& before)
	{
		build_heap(values, length, before);

		for (std::size_t unsorted = length; unsorted > 1; --unsorted)
		{
			swap_elements(values, 0, unsorted - 1);
			sift_down(values, 0, unsorted - 1, before);
		}
	}
}

namespace ds
{
	struct heap_sort
	{
		template <class sequence, class ordering>
		static void apply(sequence& values, std::size_t length, const ordering& before)
		{
			detail::heap_sort(values, length, before);
		}
	};
}
