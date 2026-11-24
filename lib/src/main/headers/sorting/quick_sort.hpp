#pragma once
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace ds::detail
{
	// The element type behind a sequence with positional access.
	template <class sequence>
	using element_of = std::remove_reference_t<decltype(std::declval<sequence&>()[0])>;

	template <class sequence>
	void swap_elements(sequence& values, std::size_t first, std::size_t second)
	{
		using std::swap;
		swap(values[first], values[second]);
	}

	template <class sequence, class ordering>
	void bubble_sort(sequence& values, std::size_t length, const ordering& before)
	{
		std::size_t unsorted = length;
		while (unsorted > 1)
		{
			std::size_t last_swap = 0;
			for (std::size_t index = 1; index < unsorted; ++index)
			{
				if (!before(values[index], values[index - 1]))
					continue;
				swap_elements(values, index - 1, index);
				last_swap = index;
			}

			unsorted = last_swap;
		}
	}

	template <class sequence, class ordering>
	void selection_sort(sequence& values, std::size_t length, const ordering& before)
	{
		for (std::size_t position = 0; position + 1 < length; ++position)
		{
			std::size_t smallest = position;
			for (std::size_t index = position + 1; index < length; ++index)
				if (before(values[index], values[smallest]))
					smallest = index;

			if (smallest != position)
				swap_elements(values, smallest, position);
		}
	}

	template <class sequence, class ordering>
	void insertion_sort(sequence& values, std::size_t length, const ordering& before)
	{
		for (std::size_t index = 1; index < length; ++index)
		{
			element_of<sequence> key = std::move(values[index]);
			std::size_t hole = index;
			for (; hole > 0 && before(key, values[hole - 1]); --hole)
				values[hole] = std::move(values[hole - 1]);
			values[hole] = std::move(key);
		}
	}

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

	// Merges the sorted runs [left, middle) and [middle, right), parking only the left run in the buffer.
	template <class sequence, class ordering>
	struct merge_context
	{
		sequence& values;
		scratch_buffer<element_of<sequence>>& buffer;
		const ordering& before;
	};

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

	template <class sequence, class ordering>
	void merge_sort(sequence& values, std::size_t length, const ordering& before)
	{
		if (length < 2)
			return;

		scratch_buffer<element_of<sequence>> buffer(length / 2 + 1);
		merge_context<sequence, ordering> context{ values, buffer, before };
		merge_sort_range(context, 0, length);
	}

	// Restores the max-heap property (by the ordering) below the start, within the first length elements.
	template <class sequence, class ordering>
	void sift_down(sequence& values, std::size_t start, std::size_t length, const ordering& before)
	{
		std::size_t parent = start;
		while (true)
		{
			std::size_t largest = parent;
			const std::size_t left = 2 * parent + 1;
			const std::size_t right = left + 1;
			if (left < length && before(values[largest], values[left]))
				largest = left;
			if (right < length && before(values[largest], values[right]))
				largest = right;
			if (largest == parent)
				return;

			swap_elements(values, parent, largest);
			parent = largest;
		}
	}

	template <class sequence, class ordering>
	void heap_sort(sequence& values, std::size_t length, const ordering& before)
	{
		for (std::size_t parent = length / 2; parent > 0; --parent)
			sift_down(values, parent - 1, length, before);

		for (std::size_t unsorted = length; unsorted > 1; --unsorted)
		{
			swap_elements(values, 0, unsorted - 1);
			sift_down(values, 0, unsorted - 1, before);
		}
	}

	// An inclusive range of positions [first, last].
	struct index_range
	{
		std::size_t first;
		std::size_t last;
	};

	template <class sequence, class ordering>
	std::size_t median_of_three_index(const sequence& values, index_range bounds, const ordering& before)
	{
		const std::size_t middle = bounds.first + (bounds.last - bounds.first) / 2;
		const auto& first = values[bounds.first];
		const auto& centre = values[middle];
		const auto& last = values[bounds.last];

		if (before(first, centre))
		{
			if (before(centre, last))
				return middle;
			return before(first, last) ? bounds.last : bounds.first;
		}

		if (before(first, last))
			return bounds.first;
		return before(centre, last) ? bounds.last : middle;
	}

	template <class sequence, class ordering>
	struct partition_context
	{
		sequence& values;
		const ordering& before;
	};

	// Hoare partition around a pivot moved to the front; returns the last index of the left part.
	template <class context_type>
	std::size_t partition(context_type& context, index_range bounds)
	{
		const std::size_t pivot_index = median_of_three_index(context.values, bounds, context.before);
		swap_elements(context.values, bounds.first, pivot_index);
		const auto pivot = context.values[bounds.first];

		std::size_t low = bounds.first;
		std::size_t high = bounds.last;
		while (true)
		{
			while (context.before(context.values[low], pivot))
				++low;
			while (context.before(pivot, context.values[high]))
				--high;
			if (low >= high)
				return high;

			swap_elements(context.values, low, high);
			++low;
			--high;
		}
	}

	// Recurses into the smaller part and loops over the larger, so the stack depth stays logarithmic.
	template <class context_type>
	void quick_sort_range(context_type& context, index_range bounds)
	{
		while (bounds.first < bounds.last)
		{
			const std::size_t split = partition(context, bounds);
			if (split - bounds.first < bounds.last - split)
			{
				quick_sort_range(context, { bounds.first, split });
				bounds.first = split + 1;
			}
			else
			{
				quick_sort_range(context, { split + 1, bounds.last });
				bounds.last = split;
			}
		}
	}

	template <class sequence, class ordering>
	void quick_sort(sequence& values, std::size_t length, const ordering& before)
	{
		if (length < 2)
			return;

		partition_context<sequence, ordering> context{ values, before };
		quick_sort_range(context, { 0, length - 1 });
	}
}

namespace ds
{
	// Sorting policies for array::sort. Stable: bubble, insertion, merge. Unstable: selection, heap, quick.
	struct bubble_sort
	{
		template <class sequence, class ordering>
		static void apply(sequence& values, std::size_t length, const ordering& before)
		{
			detail::bubble_sort(values, length, before);
		}
	};

	struct selection_sort
	{
		template <class sequence, class ordering>
		static void apply(sequence& values, std::size_t length, const ordering& before)
		{
			detail::selection_sort(values, length, before);
		}
	};

	struct insertion_sort
	{
		template <class sequence, class ordering>
		static void apply(sequence& values, std::size_t length, const ordering& before)
		{
			detail::insertion_sort(values, length, before);
		}
	};

	struct merge_sort
	{
		template <class sequence, class ordering>
		static void apply(sequence& values, std::size_t length, const ordering& before)
		{
			detail::merge_sort(values, length, before);
		}
	};

	struct heap_sort
	{
		template <class sequence, class ordering>
		static void apply(sequence& values, std::size_t length, const ordering& before)
		{
			detail::heap_sort(values, length, before);
		}
	};

	struct quick_sort
	{
		template <class sequence, class ordering>
		static void apply(sequence& values, std::size_t length, const ordering& before)
		{
			detail::quick_sort(values, length, before);
		}
	};
}
