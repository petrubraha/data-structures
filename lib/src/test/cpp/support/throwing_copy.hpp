#pragma once
#include <stdexcept>

namespace ds::test
{
	// Element whose copy constructor throws once a shared budget runs out, to test exception safety.
	class throwing_copy
	{
	public:
		// Copies allowed before the next one throws; a negative budget never throws.
		inline static int copies_left = -1;

		explicit throwing_copy(int number = 0) noexcept : number(number) {}

		throwing_copy(const throwing_copy& other) : number(other.number)
		{
			if (copies_left == 0)
				throw std::runtime_error("throwing_copy: copy budget exhausted");
			if (copies_left > 0)
				--copies_left;
		}

		throwing_copy& operator=(const throwing_copy&) = default;

		bool operator==(const throwing_copy& other) const noexcept
		{
			return number == other.number;
		}

	private:
		int number;
	};

	// Sets the copy budget for one scope and disables throwing again when the scope ends.
	class copy_budget
	{
	public:
		explicit copy_budget(int copies) noexcept
		{
			throwing_copy::copies_left = copies;
		}

		copy_budget(const copy_budget&) = delete;
		copy_budget& operator=(const copy_budget&) = delete;

		~copy_budget()
		{
			throwing_copy::copies_left = -1;
		}
	};
}
