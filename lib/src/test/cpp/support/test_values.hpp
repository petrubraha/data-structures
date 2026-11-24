#pragma once
#include <climits>
#include <cstddef>
#include <iterator>
#include "compound_type.hpp"

namespace ds::test
{
	// Negative, positive, zero, repeated and extreme values, in no particular order.
	inline constexpr int numbers[] = { -1, 7, 0, 255, 65535, INT_MIN, -7, 3, 7, INT_MAX };
	inline constexpr std::size_t number_count = std::size(numbers);

	// The same values in ascending order.
	inline constexpr int sorted_numbers[] = { INT_MIN, -7, -1, 0, 3, 7, 7, 255, 65535, INT_MAX };

	// Texts are long enough to defeat the small-string optimisation, so every copy allocates.
	inline const compound_type compounds[] = {
		{ 20, true, "compound value with the letter delta" },
		{ 18, false, "compound value with the letter alpha" },
		{ 18, true, "compound value with the letter charlie" },
		{ 19, false, "compound value with the letter bravo" },
		{ 12, true, "compound value with the letter echo" },
	};
	inline constexpr std::size_t compound_count = std::size(compounds);
}
