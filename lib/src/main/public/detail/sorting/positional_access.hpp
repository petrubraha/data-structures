#pragma once
#include <cstddef>
#include <type_traits>
#include <utility>

namespace ds::detail
{
	// The element type behind a sequence with positional access (a mutable operator[]).
	template <class sequence>
	using element_of = std::remove_reference_t<decltype(std::declval<sequence&>()[0])>;

	template <class sequence>
	void swap_elements(sequence& values, std::size_t first, std::size_t second)
	{
		using std::swap;
		swap(values[first], values[second]);
	}

	// An inclusive range of positions [first, last].
	struct index_range
	{
		std::size_t first;
		std::size_t last;
	};
}
