#pragma once
#include <cstddef>
#include <memory>
#include <new>
#include <utility>
#include "positional_access.hpp"

namespace ds::detail
{
	// Uninitialized storage for merge_sort; destroys whatever it holds when it goes out of scope.
	template <class T>
	class scratch_buffer
	{
	public:
		explicit scratch_buffer(std::size_t reserved) : capacity(reserved), elements(allocator.allocate(reserved)) {}
		scratch_buffer(const scratch_buffer&) = delete;
		scratch_buffer& operator=(const scratch_buffer&) = delete;

		~scratch_buffer()
		{
			clear();
			allocator.deallocate(elements, capacity);
		}

		T& operator[](std::size_t index) noexcept
		{
			return elements[index];
		}

		void push(T&& value)
		{
			::new (static_cast<void*>(elements + length)) T(std::move(value));
			++length;
		}

		void clear() noexcept
		{
			std::destroy_n(elements, length);
			length = 0;
		}

	private:
		std::allocator<T> allocator;
		std::size_t capacity;
		std::size_t length = 0;
		T* elements;
	};

	template <class sequence, class ordering>
	struct merge_context
	{
		sequence& values;
		scratch_buffer<element_of<sequence>>& buffer;
		const ordering& before;
	};

	// Merges the sorted runs [left, middle) and [middle, right), parking only the left run in the buffer.
	// Ties take the left element, which keeps the sort stable.
	template <class context_type>
	void merge_runs(context_type& context, std::size_t left, std::size_t middle, std::size_t right)
	{
		for (std::size_t index = left; index < middle; ++index)
			context.buffer.push(std::move(context.values[index]));

		const std::size_t parked = middle - left;
		std::size_t from_left = 0;
		std::size_t from_right = middle;
		std::size_t destination = left;
		while (from_left < parked && from_right < right)
		{
			if (context.before(context.values[from_right], context.buffer[from_left]))
				context.values[destination++] = std::move(context.values[from_right++]);
			else
				context.values[destination++] = std::move(context.buffer[from_left++]);
		}

		while (from_left < parked)
			context.values[destination++] = std::move(context.buffer[from_left++]);
		context.buffer.clear();
	}

	template <class context_type>
	void merge_sort_range(context_type& context, std::size_t left, std::size_t right)
	{
		if (right - left < 2)
			return;

		const std::size_t middle = left + (right - left) / 2;
		merge_sort_range(context, left, middle);
		merge_sort_range(context, middle, right);
		merge_runs(context, left, middle, right);
	}

	// Stable. Allocates one buffer of half the length for the whole sort.
	template <class sequence, class ordering>
	void merge_sort(sequence& values, std::size_t length, const ordering& before)
	{
		if (length < 2)
			return;

		scratch_buffer<element_of<sequence>> buffer(length / 2 + 1);
		merge_context<sequence, ordering> context{ values, buffer, before };
		merge_sort_range(context, 0, length);
	}
}

namespace ds
{
	struct merge_sort
	{
		template <class sequence, class ordering>
		static void apply(sequence& values, std::size_t length, const ordering& before)
		{
			detail::merge_sort(values, length, before);
		}
	};
}
