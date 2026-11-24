#pragma once
#include <cstddef>
#include <initializer_list>
#include <utility>
#include "linked_list.hpp"
#include "utils/exceptions.hpp"

namespace ds
{
	// First in, first out. Holds a linked list: push links at the tail, pop unlinks the head, both O(1).
	template <class T>
	class queue
	{
	public:
		using value_type = T;
		using size_type = std::size_t;
		using const_reference = const T&;

		// Constructors and destructor. The source elements are pushed in order, so the first one is in front.
		~queue() = default;
		queue() = default;
		queue(std::initializer_list<T> initial);
		queue(const T* source, size_type count);
		queue(const queue& other) = default;
		queue(queue&& other) noexcept = default;

		// Modifier methods.
		queue& operator=(const queue& other) = default;
		queue& operator=(queue&& other) noexcept = default;

		// Specific methods.
		queue& push(T value);
		queue& pop();

		// Constant methods.
		bool operator==(const queue& other) const;
		bool operator!=(const queue& other) const;
		size_type size() const noexcept;
		bool empty() const noexcept;
		const_reference front() const;
		const_reference back() const;

	private:
		linked_list<T> values;
	};

	//------------------------------------------------
	// Constructors and destructor.

	template <class T>
	queue<T>::queue(std::initializer_list<T> initial) : values(initial) {}

	template <class T>
	queue<T>::queue(const T* source, size_type count) : values(source, count) {}

	//------------------------------------------------
	// Specific methods.

	template <class T>
	queue<T>& queue<T>::push(T value)
	{
		values.insert(std::move(value));
		return *this;
	}

	template <class T>
	queue<T>& queue<T>::pop()
	{
		if (empty())
			throw empty_error("queue::pop");
		values.remove(0);
		return *this;
	}

	//------------------------------------------------
	// Constant methods.

	template <class T>
	bool queue<T>::operator==(const queue& other) const
	{
		return values == other.values;
	}

	template <class T>
	bool queue<T>::operator!=(const queue& other) const
	{
		return values != other.values;
	}

	template <class T>
	typename queue<T>::size_type queue<T>::size() const noexcept
	{
		return values.size();
	}

	template <class T>
	bool queue<T>::empty() const noexcept
	{
		return values.empty();
	}

	template <class T>
	typename queue<T>::const_reference queue<T>::front() const
	{
		if (empty())
			throw empty_error("queue::front");
		return values.front();
	}

	template <class T>
	typename queue<T>::const_reference queue<T>::back() const
	{
		if (empty())
			throw empty_error("queue::back");
		return values.back();
	}
}
