#pragma once

namespace ds::detail
{
	// A half-open range [first, last) of positions satisfying the iterator concept (*, ++, !=).
	template <class position>
	struct position_range
	{
		position first;
		position last;
	};

	// Swaps the arguments of an ordering, so "ordered before" becomes "ordered after".
	template <class ordering>
	class reversed
	{
	public:
		explicit reversed(const ordering& order) noexcept : order(order) {}

		template <class T>
		bool operator()(const T& left, const T& right) const
		{
			return order(right, left);
		}

	private:
		const ordering& order;
	};

	// The first element equal to the value, or the end of the range.
	template <class position, class T>
	position find(position_range<position> range, const T& value)
	{
		for (position current = range.first; current != range.last; ++current)
			if (*current == value)
				return current;
		return range.last;
	}

	// The first element that no other element is ordered before. The range must not be empty.
	template <class position, class ordering>
	position first_extreme(position_range<position> range, const ordering& before)
	{
		position best = range.first;
		for (position current = range.first; current != range.last; ++current)
			if (before(*current, *best))
				best = current;
		return best;
	}

	// The element ordered before the target and closest to it, or the end of the range.
	template <class position, class T, class ordering>
	position closest_before(position_range<position> range, const T& target, const ordering& before)
	{
		position best = range.last;
		bool found = false;
		for (position current = range.first; current != range.last; ++current)
		{
			if (!before(*current, target))
				continue;
			if (found && !before(*best, *current))
				continue;

			best = current;
			found = true;
		}

		return best;
	}
}
