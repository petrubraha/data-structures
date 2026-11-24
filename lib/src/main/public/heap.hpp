#pragma once
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include "array.hpp"
#include "policy/order.hpp"
#include "sorting/heap_sort.hpp"
#include "utils/constants.hpp"
#include "utils/conversion.hpp"
#include "utils/exceptions.hpp"
#include "utils/printing.hpp"

namespace ds
{
	// Binary heap stored level by level in an array: the parent of i is (i - 1) / 2, its children 2i + 1
	// and 2i + 2. The order tag (max or min) decides which end of the comparator stays on top.
	template <class order, class T, class compare = std::less<T>>
	class heap
	{
	public:
		using value_type = T;
		using size_type = std::size_t;
		using const_reference = const T&;
		using comparator = compare;
		using const_iterator = typename array<T>::const_iterator;

		// Constructors and destructor.
		~heap() = default;
		heap() = default;
		heap(std::initializer_list<T> initial);
		heap(const T* source, size_type count);
		heap(const heap& other) = default;
		heap(heap&& other) noexcept(std::is_nothrow_move_constructible_v<comparator>) = default;

		// Modifier methods.
		heap& operator=(const heap& other) = default;
		heap& operator=(heap&& other) noexcept(std::is_nothrow_move_assignable_v<comparator>) = default;
		heap& clear() noexcept;
		heap& set_compare(comparator replacement);

		// Specific methods.
		heap& insert(T value);
		T extract();
		heap& replace(size_type index, T value);

		// Constant methods.
		bool operator==(const heap& other) const;
		bool operator!=(const heap& other) const;
		size_type size() const noexcept;
		bool empty() const noexcept;
		void print() const;
		const_reference peek() const;

		// Query operations.
		size_type search(const_reference value) const;

		// Iterator methods. Traversal is level by level; elements are read-only to keep the heap valid.
		const_iterator begin() const noexcept;
		const_iterator end() const noexcept;

		// Friend functions.
		friend std::ostream& operator<<(std::ostream& out, const heap& elements)
		{
			return detail::write_elements(out, elements);
		}

		friend std::unique_ptr<T[]> convert(const heap& elements)
		{
			return detail::copy_elements<T>(elements, elements.size());
		}

	private:
		array<T> values;
		comparator ordering{};

		bool rises_above(const T& candidate, const T& parent) const;
		void sift_up(size_type index);
		void sift_down(size_type index);
		size_type search_below(size_type index, const T& value) const;

	public:
		static constexpr size_type npos = detail::npos;
	};

	//------------------------------------------------
	// Constructors and destructor.

	template <class order, class T, class compare>
	heap<order, T, compare>::heap(std::initializer_list<T> initial) : heap(initial.begin(), initial.size()) {}

	template <class order, class T, class compare>
	heap<order, T, compare>::heap(const T* source, size_type count) : values(source, count)
	{
		detail::build_heap(values, values.size(), order::sift_order(ordering));
	}

	//------------------------------------------------
	// Modifier methods.

	template <class order, class T, class compare>
	heap<order, T, compare>& heap<order, T, compare>::clear() noexcept
	{
		values.clear();
		return *this;
	}

	template <class order, class T, class compare>
	heap<order, T, compare>& heap<order, T, compare>::set_compare(comparator replacement)
	{
		ordering = std::move(replacement);
		detail::build_heap(values, values.size(), order::sift_order(ordering));
		return *this;
	}

	//------------------------------------------------
	// Specific methods.

	template <class order, class T, class compare>
	heap<order, T, compare>& heap<order, T, compare>::insert(T value)
	{
		values.insert(std::move(value));
		sift_up(values.size() - 1);
		return *this;
	}

	template <class order, class T, class compare>
	T heap<order, T, compare>::extract()
	{
		if (empty())
			throw empty_error("heap::extract");

		const size_type last = values.size() - 1;
		detail::swap_elements(values, 0, last);
		T top = std::move(values[last]);
		values.remove(last);
		sift_down(0);
		return top;
	}

	template <class order, class T, class compare>
	heap<order, T, compare>& heap<order, T, compare>::replace(size_type index, T value)
	{
		if (index >= values.size())
			throw index_error("heap::replace", index, values.size());

		values[index] = std::move(value);
		const bool rises = index > 0 && rises_above(values[index], values[(index - 1) / 2]);
		if (rises)
			sift_up(index);
		else
			sift_down(index);
		return *this;
	}

	//------------------------------------------------
	// Constant methods.

	template <class order, class T, class compare>
	bool heap<order, T, compare>::operator==(const heap& other) const
	{
		return values == other.values;
	}

	template <class order, class T, class compare>
	bool heap<order, T, compare>::operator!=(const heap& other) const
	{
		return values != other.values;
	}

	template <class order, class T, class compare>
	typename heap<order, T, compare>::size_type heap<order, T, compare>::size() const noexcept
	{
		return values.size();
	}

	template <class order, class T, class compare>
	bool heap<order, T, compare>::empty() const noexcept
	{
		return values.empty();
	}

	template <class order, class T, class compare>
	void heap<order, T, compare>::print() const
	{
		std::cout << *this << '\n';
	}

	template <class order, class T, class compare>
	typename heap<order, T, compare>::const_reference heap<order, T, compare>::peek() const
	{
		if (empty())
			throw empty_error("heap::peek");
		return values[0];
	}

	//------------------------------------------------
	// Query operations.

	template <class order, class T, class compare>
	typename heap<order, T, compare>::size_type heap<order, T, compare>::search(const_reference value) const
	{
		return empty() ? npos : search_below(0, value);
	}

	//------------------------------------------------
	// Iterator methods.

	template <class order, class T, class compare>
	typename heap<order, T, compare>::const_iterator heap<order, T, compare>::begin() const noexcept
	{
		return values.begin();
	}

	template <class order, class T, class compare>
	typename heap<order, T, compare>::const_iterator heap<order, T, compare>::end() const noexcept
	{
		return values.end();
	}

	//------------------------------------------------
	// Private helpers.

	template <class order, class T, class compare>
	bool heap<order, T, compare>::rises_above(const T& candidate, const T& parent) const
	{
		const auto& sift_order = order::sift_order(ordering);
		return sift_order(parent, candidate);
	}

	template <class order, class T, class compare>
	void heap<order, T, compare>::sift_up(size_type index)
	{
		while (index > 0)
		{
			const size_type parent = (index - 1) / 2;
			if (!rises_above(values[index], values[parent]))
				return;

			detail::swap_elements(values, index, parent);
			index = parent;
		}
	}

	template <class order, class T, class compare>
	void heap<order, T, compare>::sift_down(size_type index)
	{
		detail::sift_down(values, index, values.size(), order::sift_order(ordering));
	}

	// Depth-first search that skips every subtree whose root the value would rise above: by the heap
	// property, nothing below that root can equal the value.
	template <class order, class T, class compare>
	typename heap<order, T, compare>::size_type heap<order, T, compare>::search_below(size_type index, const T& value) const
	{
		if (index >= values.size() || rises_above(value, values[index]))
			return npos;
		if (values[index] == value)
			return index;

		const size_type in_left = search_below(2 * index + 1, value);
		return in_left != npos ? in_left : search_below(2 * index + 2, value);
	}
}
