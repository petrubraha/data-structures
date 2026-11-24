#pragma once
#include <type_traits>
#include "utils/exceptions.hpp"

namespace ds::detail
{
	// Walks a contiguous block. A const-qualified T gives read-only traversal.
	template <class T>
	class array_iterator
	{
	public:
		array_iterator(T* current, T* boundary) noexcept;

		// Lets a mutable iterator convert to its read-only counterpart.
		template <class mutable_type, class = std::enable_if_t<std::is_same_v<const mutable_type, T>>>
		array_iterator(const array_iterator<mutable_type>& other) noexcept : current(other.current), boundary(other.boundary) {}

		T& operator*() const noexcept;
		array_iterator& operator++() noexcept;
		bool operator!=(const array_iterator& other) const noexcept;

	private:
		template <class>
		friend class array_iterator;

		T* current;
		T* boundary;
	};

	//------------------------------------------------

	template <class T>
	array_iterator<T>::array_iterator(T* current, T* boundary) noexcept : current(current), boundary(boundary) {}

	template <class T>
	T& array_iterator<T>::operator*() const noexcept
	{
		if (current == boundary)
			fail_fast("array_iterator::operator*: dereferenced past the end");
		return *current;
	}

	template <class T>
	array_iterator<T>& array_iterator<T>::operator++() noexcept
	{
		if (current == boundary)
			fail_fast("array_iterator::operator++: advanced past the end");
		++current;
		return *this;
	}

	template <class T>
	bool array_iterator<T>::operator!=(const array_iterator& other) const noexcept
	{
		return current != other.current;
	}
}
