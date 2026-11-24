#pragma once
#include <ostream>

namespace ds::detail
{
	// Writes the elements separated by single spaces, with no trailing separator.
	template <class range>
	std::ostream& write_elements(std::ostream& out, const range& elements)
	{
		bool first = true;
		for (const auto& element : elements)
		{
			if (!first)
				out << ' ';
			out << element;
			first = false;
		}

		return out;
	}
}
