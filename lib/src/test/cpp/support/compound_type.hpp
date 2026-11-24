#pragma once
#include <cstddef>
#include <functional>
#include <ostream>
#include <string>
#include <utility>

namespace ds::test
{
	// Multi-field element with no natural order and heap-owned text; tests order it through the comparators below.
	class compound_type
	{
	public:
		compound_type() = default;
		compound_type(int number, bool flag, std::string text);

		int get_number() const noexcept;
		bool get_flag() const noexcept;
		const std::string& get_text() const noexcept;

		bool operator==(const compound_type& other) const;
		bool operator!=(const compound_type& other) const;

		friend std::ostream& operator<<(std::ostream& out, const compound_type& value)
		{
			return out << '{' << value.number << ", " << value.flag << ", " << value.text << '}';
		}

	private:
		int number = 0;
		bool flag = false;
		std::string text;
	};

	struct by_number
	{
		bool operator()(const compound_type& left, const compound_type& right) const;
	};

	struct by_flag
	{
		bool operator()(const compound_type& left, const compound_type& right) const;
	};

	struct by_text
	{
		bool operator()(const compound_type& left, const compound_type& right) const;
	};

	struct compound_hash
	{
		std::size_t operator()(const compound_type& value) const;
	};

	//------------------------------------------------

	inline compound_type::compound_type(int number, bool flag, std::string text)
		: number(number), flag(flag), text(std::move(text)) {}

	inline int compound_type::get_number() const noexcept
	{
		return number;
	}

	inline bool compound_type::get_flag() const noexcept
	{
		return flag;
	}

	inline const std::string& compound_type::get_text() const noexcept
	{
		return text;
	}

	inline bool compound_type::operator==(const compound_type& other) const
	{
		return number == other.number && flag == other.flag && text == other.text;
	}

	inline bool compound_type::operator!=(const compound_type& other) const
	{
		return !(*this == other);
	}

	inline bool by_number::operator()(const compound_type& left, const compound_type& right) const
	{
		return left.get_number() < right.get_number();
	}

	inline bool by_flag::operator()(const compound_type& left, const compound_type& right) const
	{
		return left.get_flag() < right.get_flag();
	}

	inline bool by_text::operator()(const compound_type& left, const compound_type& right) const
	{
		return left.get_text() < right.get_text();
	}

	inline std::size_t compound_hash::operator()(const compound_type& value) const
	{
		return std::hash<std::string>()(value.get_text()) ^ std::hash<int>()(value.get_number());
	}
}
