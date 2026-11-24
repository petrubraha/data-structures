#pragma once
#include <cstddef>
#include <random>
#include "positional_access.hpp"

// Pivot strategies for quick_sort. Each returns an index inside the inclusive bounds.
namespace ds
{
	struct first
	{
		template <class sequence, class ordering>
		std::size_t operator()(const sequence&, detail::index_range bounds, const ordering&) const noexcept
		{
			return bounds.first;
		}
	};

	struct last
	{
		template <class sequence, class ordering>
		std::size_t operator()(const sequence&, detail::index_range bounds, const ordering&) const noexcept
		{
			return bounds.last;
		}
	};

	struct middle
	{
		template <class sequence, class ordering>
		std::size_t operator()(const sequence&, detail::index_range bounds, const ordering&) const noexcept
		{
			return bounds.first + (bounds.last - bounds.first) / 2;
		}
	};

	// The median of the first, middle and last elements; guards against sorted and reversed input.
	struct median_of_three
	{
		template <class sequence, class ordering>
		std::size_t operator()(const sequence& values, detail::index_range bounds, const ordering& before) const
		{
			const std::size_t centre = middle()(values, bounds, before);
			const auto& first_value = values[bounds.first];
			const auto& centre_value = values[centre];
			const auto& last_value = values[bounds.last];

			if (before(first_value, centre_value))
			{
				if (before(centre_value, last_value))
					return centre;
				return before(first_value, last_value) ? bounds.last : bounds.first;
			}

			if (before(first_value, last_value))
				return bounds.first;
			return before(centre_value, last_value) ? bounds.last : centre;
		}
	};

	// A uniformly random index; the generator is seeded once per sort.
	class random
	{
	public:
		random() : generator(std::random_device()()) {}

		template <class sequence, class ordering>
		std::size_t operator()(const sequence&, detail::index_range bounds, const ordering&)
		{
			std::uniform_int_distribution<std::size_t> distribution(bounds.first, bounds.last);
			return distribution(generator);
		}

	private:
		std::minstd_rand generator;
	};
}
