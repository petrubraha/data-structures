#pragma once
#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include "iterator/array_iterator.hpp"
#include "sorting/bubble_sort.hpp"
#include "sorting/heap_sort.hpp"
#include "sorting/insertion_sort.hpp"
#include "sorting/merge_sort.hpp"
#include "sorting/quick_sort.hpp"
#include "sorting/selection_sort.hpp"
#include "utils/constants.hpp"
#include "utils/conversion.hpp"
#include "utils/exceptions.hpp"
#include "utils/linear_queries.hpp"
#include "utils/printing.hpp"

namespace ds
{
	// Contiguous sequence that grows on demand. The comparator orders elements for sort and the queries.
	template <class T, class compare = std::less<T>>
	class array
	{
	public:
		using value_type = T;
		using size_type = std::size_t;
		using reference = T&;
		using const_reference = const T&;
		using comparator = compare;
		using iterator = detail::array_iterator<T>;
		using const_iterator = detail::array_iterator<const T>;

		// Constructors and destructor.
		~array();
		array() noexcept(std::is_nothrow_default_constructible_v<comparator>);
		array(std::initializer_list<T> initial);
		array(const T* source, size_type count);
		explicit array(size_type reserved);
		array(const array& other);
		array(array&& other) noexcept(std::is_nothrow_move_constructible_v<comparator>);

		// Modifier methods.
		array& operator=(const array& other);
		array& operator=(array&& other) noexcept(std::is_nothrow_move_assignable_v<comparator>);
		array& clear() noexcept;
		array& set_compare(comparator replacement);

		// Specific methods.
		array& insert(T value);
		array& insert(T value, size_type index);
		array& remove(size_type index);
		template <class algorithm = quick_sort, class... options>
		array& sort();

		// Constant methods.
		bool operator==(const array& other) const;
		bool operator!=(const array& other) const;
		size_type size() const noexcept;
		size_type capacity() const noexcept;
		bool empty() const noexcept;
		void print() const;
		const_reference get(size_type index) const;
		reference operator[](size_type index);
		const_reference operator[](size_type index) const;

		// Query operations.
		size_type search(const_reference value) const;
		size_type minimum() const;
		size_type maximum() const;
		size_type predecessor(size_type index) const;
		size_type successor(size_type index) const;

		// Iterator methods.
		iterator begin() noexcept;
		iterator end() noexcept;
		const_iterator begin() const noexcept;
		const_iterator end() const noexcept;

		// Friend functions.
		friend std::ostream& operator<<(std::ostream& out, const array& sequence)
		{
			return detail::write_elements(out, sequence);
		}

		friend std::unique_ptr<T[]> convert(const array& sequence)
		{
			return detail::copy_elements<T>(sequence, sequence.length);
		}

	private:
		T* values = nullptr;
		size_type length = 0;
		size_type allocated = 0;
		comparator order{};

		static size_type maximum_capacity() noexcept;
		void construct_from(const T* source, size_type count);
		void reserve(const char* operation, size_type required);
		void reallocate(size_type replacement_capacity);
		void release() noexcept;
		void swap(array& other) noexcept;
		void check_index(const char* operation, size_type index) const;
		detail::position_range<const T*> stored() const noexcept;
		size_type index_of(const T* position) const noexcept;

	public:
		static constexpr size_type npos = detail::npos;
		static constexpr size_type default_capacity = detail::default_capacity;
	};

	//------------------------------------------------
	// Constructors and destructor.

	template <class T, class compare>
	array<T, compare>::~array()
	{
		release();
	}

	template <class T, class compare>
	array<T, compare>::array() noexcept(std::is_nothrow_default_constructible_v<comparator>) {}

	template <class T, class compare>
	array<T, compare>::array(std::initializer_list<T> initial) : array(initial.begin(), initial.size()) {}

	template <class T, class compare>
	array<T, compare>::array(const T* source, size_type count)
	{
		construct_from(source, count);
	}

	template <class T, class compare>
	array<T, compare>::array(size_type reserved)
	{
		if (reserved > maximum_capacity())
			throw capacity_error("array::array", reserved, maximum_capacity());
		reallocate(reserved);
	}

	template <class T, class compare>
	array<T, compare>::array(const array& other) : order(other.order)
	{
		construct_from(other.values, other.length);
	}

	template <class T, class compare>
	array<T, compare>::array(array&& other) noexcept(std::is_nothrow_move_constructible_v<comparator>)
		: values(std::exchange(other.values, nullptr)), length(std::exchange(other.length, 0)),
		allocated(std::exchange(other.allocated, 0)), order(std::move(other.order)) {}

	//------------------------------------------------
	// Modifier methods.

	template <class T, class compare>
	array<T, compare>& array<T, compare>::operator=(const array& other)
	{
		array copy(other);
		swap(copy);
		return *this;
	}

	template <class T, class compare>
	array<T, compare>& array<T, compare>::operator=(array&& other) noexcept(std::is_nothrow_move_assignable_v<comparator>)
	{
		if (this == &other)
			return *this;

		release();
		values = std::exchange(other.values, nullptr);
		length = std::exchange(other.length, 0);
		allocated = std::exchange(other.allocated, 0);
		order = std::move(other.order);
		return *this;
	}

	template <class T, class compare>
	array<T, compare>& array<T, compare>::clear() noexcept
	{
		std::destroy_n(values, length);
		length = 0;
		return *this;
	}

	template <class T, class compare>
	array<T, compare>& array<T, compare>::set_compare(comparator replacement)
	{
		order = std::move(replacement);
		return *this;
	}

	//------------------------------------------------
	// Specific methods.

	template <class T, class compare>
	array<T, compare>& array<T, compare>::insert(T value)
	{
		return insert(std::move(value), length);
	}

	template <class T, class compare>
	array<T, compare>& array<T, compare>::insert(T value, size_type index)
	{
		if (index > length)
			throw index_error("array::insert", index, length);

		reserve("array::insert", length + 1);
		if (index == length)
		{
			::new (static_cast<void*>(values + length)) T(std::move(value));
			++length;
			return *this;
		}

		::new (static_cast<void*>(values + length)) T(std::move(values[length - 1]));
		++length;
		std::move_backward(values + index, values + length - 2, values + length - 1);
		values[index] = std::move(value);
		return *this;
	}

	template <class T, class compare>
	array<T, compare>& array<T, compare>::remove(size_type index)
	{
		check_index("array::remove", index);
		std::move(values + index + 1, values + length, values + index);
		std::destroy_at(values + length - 1);
		--length;
		return *this;
	}

	template <class T, class compare>
	template <class algorithm, class... options>
	array<T, compare>& array<T, compare>::sort()
	{
		algorithm::template apply<options...>(values, length, order);
		return *this;
	}

	//------------------------------------------------
	// Constant methods.

	template <class T, class compare>
	bool array<T, compare>::operator==(const array& other) const
	{
		return length == other.length && std::equal(values, values + length, other.values);
	}

	template <class T, class compare>
	bool array<T, compare>::operator!=(const array& other) const
	{
		return !(*this == other);
	}

	template <class T, class compare>
	typename array<T, compare>::size_type array<T, compare>::size() const noexcept
	{
		return length;
	}

	template <class T, class compare>
	typename array<T, compare>::size_type array<T, compare>::capacity() const noexcept
	{
		return allocated;
	}

	template <class T, class compare>
	bool array<T, compare>::empty() const noexcept
	{
		return length == 0;
	}

	template <class T, class compare>
	void array<T, compare>::print() const
	{
		std::cout << *this << '\n';
	}

	template <class T, class compare>
	typename array<T, compare>::const_reference array<T, compare>::get(size_type index) const
	{
		check_index("array::get", index);
		return values[index];
	}

	template <class T, class compare>
	typename array<T, compare>::reference array<T, compare>::operator[](size_type index)
	{
		check_index("array::operator[]", index);
		return values[index];
	}

	template <class T, class compare>
	typename array<T, compare>::const_reference array<T, compare>::operator[](size_type index) const
	{
		check_index("array::operator[]", index);
		return values[index];
	}

	//------------------------------------------------
	// Query operations.

	template <class T, class compare>
	typename array<T, compare>::size_type array<T, compare>::search(const_reference value) const
	{
		return index_of(detail::find(stored(), value));
	}

	template <class T, class compare>
	typename array<T, compare>::size_type array<T, compare>::minimum() const
	{
		if (empty())
			throw empty_error("array::minimum");
		return index_of(detail::first_extreme(stored(), order));
	}

	template <class T, class compare>
	typename array<T, compare>::size_type array<T, compare>::maximum() const
	{
		if (empty())
			throw empty_error("array::maximum");
		return index_of(detail::first_extreme(stored(), detail::reversed<comparator>(order)));
	}

	template <class T, class compare>
	typename array<T, compare>::size_type array<T, compare>::predecessor(size_type index) const
	{
		check_index("array::predecessor", index);
		return index_of(detail::closest_before(stored(), values[index], order));
	}

	template <class T, class compare>
	typename array<T, compare>::size_type array<T, compare>::successor(size_type index) const
	{
		check_index("array::successor", index);
		return index_of(detail::closest_before(stored(), values[index], detail::reversed<comparator>(order)));
	}

	//------------------------------------------------
	// Iterator methods.

	template <class T, class compare>
	typename array<T, compare>::iterator array<T, compare>::begin() noexcept
	{
		return iterator(values, values + length);
	}

	template <class T, class compare>
	typename array<T, compare>::iterator array<T, compare>::end() noexcept
	{
		return iterator(values + length, values + length);
	}

	template <class T, class compare>
	typename array<T, compare>::const_iterator array<T, compare>::begin() const noexcept
	{
		return const_iterator(values, values + length);
	}

	template <class T, class compare>
	typename array<T, compare>::const_iterator array<T, compare>::end() const noexcept
	{
		return const_iterator(values + length, values + length);
	}

	//------------------------------------------------
	// Private helpers.

	template <class T, class compare>
	typename array<T, compare>::size_type array<T, compare>::maximum_capacity() noexcept
	{
		return std::allocator_traits<std::allocator<T>>::max_size(std::allocator<T>());
	}

	template <class T, class compare>
	void array<T, compare>::construct_from(const T* source, size_type count)
	{
		if (count == 0)
			return;
		if (count > maximum_capacity())
			throw capacity_error("array::array", count, maximum_capacity());

		reallocate(count);
		std::uninitialized_copy_n(source, count, values);
		length = count;
	}

	template <class T, class compare>
	void array<T, compare>::reserve(const char* operation, size_type required)
	{
		if (required <= allocated)
			return;
		if (required > maximum_capacity())
			throw capacity_error(operation, required, maximum_capacity());

		const size_type grown = std::max({ required, allocated * detail::growth_factor, default_capacity });
		reallocate(std::min(grown, maximum_capacity()));
	}

	// Moves the elements into a fresh block; falls back to copying when a throwing move could lose elements.
	template <class T, class compare>
	void array<T, compare>::reallocate(size_type replacement_capacity)
	{
		std::allocator<T> allocator;
		T* fresh = allocator.allocate(replacement_capacity);
		try
		{
			if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>)
				std::uninitialized_move_n(values, length, fresh);
			else
				std::uninitialized_copy_n(values, length, fresh);
		}
		catch (...)
		{
			allocator.deallocate(fresh, replacement_capacity);
			throw;
		}

		const size_type kept = length;
		release();
		values = fresh;
		length = kept;
		allocated = replacement_capacity;
	}

	template <class T, class compare>
	void array<T, compare>::release() noexcept
	{
		std::destroy_n(values, length);
		if (values != nullptr)
			std::allocator<T>().deallocate(values, allocated);

		values = nullptr;
		length = 0;
		allocated = 0;
	}

	template <class T, class compare>
	void array<T, compare>::swap(array& other) noexcept
	{
		using std::swap;
		swap(values, other.values);
		swap(length, other.length);
		swap(allocated, other.allocated);
		swap(order, other.order);
	}

	template <class T, class compare>
	void array<T, compare>::check_index(const char* operation, size_type index) const
	{
		if (index >= length)
			throw index_error(operation, index, length);
	}

	template <class T, class compare>
	detail::position_range<const T*> array<T, compare>::stored() const noexcept
	{
		return { values, values + length };
	}

	template <class T, class compare>
	typename array<T, compare>::size_type array<T, compare>::index_of(const T* position) const noexcept
	{
		return position == values + length ? npos : static_cast<size_type>(position - values);
	}
}
