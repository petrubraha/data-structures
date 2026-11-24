#pragma once
#include <cstddef>
#include <initializer_list>
#include <utility>
#include "linked_list.hpp"
#include "utils/exceptions.hpp"

namespace ds
{
	// Last in, first out. Holds a linked list and uses its front as the top, so push and pop are O(1).
	template <class T>
	class stack
	{
	public:
		using value_type = T;
		using size_type = std::size_t;
		using const_reference = const T&;

		// Constructors and destructor. The source elements are pushed in order, so the last one is on top.
		~stack() = default;
		stack() = default;
		stack(std::initializer_list<T> initial);
		stack(const T* source, size_type count);
		stack(const stack& other) = default;
		stack(stack&& other) noexcept = default;

		// Modifier methods.
		stack& operator=(const stack& other) = default;
		stack& operator=(stack&& other) noexcept = default;

		// Specific methods.
		stack& push(T value);
		stack& pop();

		// Constant methods.
		bool operator==(const stack& other) const;
		bool operator!=(const stack& other) const;
		size_type size() const noexcept;
		bool empty() const noexcept;
		const_reference top() const;

	private:
		linked_list<T> values;
	};

	//------------------------------------------------
	// Constructors and destructor.

	template <class T>
	stack<T>::stack(std::initializer_list<T> initial) : stack(initial.begin(), initial.size()) {}

	template <class T>
	stack<T>::stack(const T* source, size_type count)
	{
		for (size_type index = 0; index < count; ++index)
			push(source[index]);
	}

	//------------------------------------------------
	// Specific methods.

	template <class T>
	stack<T>& stack<T>::push(T value)
	{
		values.insert(std::move(value), 0);
		return *this;
	}

	template <class T>
	stack<T>& stack<T>::pop()
	{
		if (empty())
			throw empty_error("stack::pop");
		values.remove(0);
		return *this;
	}

	//------------------------------------------------
	// Constant methods.

	template <class T>
	bool stack<T>::operator==(const stack& other) const
	{
		return values == other.values;
	}

	template <class T>
	bool stack<T>::operator!=(const stack& other) const
	{
		return values != other.values;
	}

	template <class T>
	typename stack<T>::size_type stack<T>::size() const noexcept
	{
		return values.size();
	}

	template <class T>
	bool stack<T>::empty() const noexcept
	{
		return values.empty();
	}

	template <class T>
	typename stack<T>::const_reference stack<T>::top() const
	{
		if (empty())
			throw empty_error("stack::top");
		return values.front();
	}
}
