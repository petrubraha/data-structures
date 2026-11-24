#pragma once
#include <cstddef>
#include <memory>

namespace ds::detail
{
	// Copies the elements, in traversal order, into a newly allocated block of the given length.
	template <class element, class range>
	std::unique_ptr<element[]> copy_elements(const range& elements, std::size_t length)
	{
		auto block = std::make_unique<element[]>(length);
		std::size_t index = 0;
		for (const auto& value : elements)
			block[index++] = value;
		return block;
	}
}
