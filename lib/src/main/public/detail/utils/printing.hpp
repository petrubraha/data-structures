#pragma once
#include <ostream>

namespace ds::detail
{
	// Writes each element through the writer, separated by single spaces, with no trailing separator.
	template <class range, class writer>
	std::ostream& write_elements(std::ostream& out, const range& elements, const writer& write_one)
	{
		bool first = true;
		for (const auto& element : elements)
		{
			if (!first)
				out << ' ';
			write_one(out, element);
			first = false;
		}

		return out;
	}

	// Writes the elements with their own stream operator, separated by single spaces.
	template <class range>
	std::ostream& write_elements(std::ostream& out, const range& elements)
	{
		const auto stream_one = [](std::ostream& target, const auto& element) { target << element; };
		return write_elements(out, elements, stream_one);
	}
}
