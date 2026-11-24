#pragma once
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <utility>
#include "detail/iterator/list_iterator.hpp"
#include "detail/node/deque_node.hpp"
#include "detail/utils/conversion.hpp"
#include "detail/utils/exceptions.hpp"
#include "detail/utils/linear_queries.hpp"
#include "detail/utils/printing.hpp"

namespace ds
{
	// Double-ended queue on a doubly linked chain: push and pop are O(1) at both ends.
	template <class T>
	class deque
	{
		using node_type = detail::deque_node<T>;

	public:
		using value_type = T;
		using size_type = std::size_t;
		using reference = T&;
		using const_reference = const T&;
		using iterator = detail::list_iterator<node_type, T>;
		using const_iterator = detail::list_iterator<node_type, const T>;

		// Constructors and destructor.
		~deque();
		deque() noexcept = default;
		deque(std::initializer_list<T> initial);
		deque(const T* source, size_type count);
		deque(const deque& other);
		deque(deque&& other) noexcept;

		// Modifier methods.
		deque& operator=(const deque& other);
		deque& operator=(deque&& other) noexcept;
		deque& clear() noexcept;

		// Specific methods.
		deque& push_front(T value);
		deque& push_back(T value);
		deque& pop_front();
		deque& pop_back();

		// Constant methods.
		bool operator==(const deque& other) const;
		bool operator!=(const deque& other) const;
		size_type size() const noexcept;
		bool empty() const noexcept;
		const_reference front() const;
		const_reference back() const;

		// Iterator methods.
		iterator begin() noexcept;
		iterator end() noexcept;
		const_iterator begin() const noexcept;
		const_iterator end() const noexcept;

		// Friend functions.
		friend std::ostream& operator<<(std::ostream& out, const deque& sequence)
		{
			return detail::write_elements(out, sequence);
		}

		friend std::unique_ptr<T[]> convert(const deque& sequence)
		{
			return detail::copy_elements<T>(sequence, sequence.length);
		}

	private:
		node_type* head = nullptr;
		node_type* tail = nullptr;
		size_type length = 0;

		template <class position>
		void append_all(detail::position_range<position> source);
		void unlink(node_type* removed) noexcept;
		void steal(deque& other) noexcept;
		void swap(deque& other) noexcept;
	};

	//------------------------------------------------
	// Constructors and destructor.

	template <class T>
	deque<T>::~deque()
	{
		clear();
	}

	template <class T>
	deque<T>::deque(std::initializer_list<T> initial) : deque(initial.begin(), initial.size()) {}

	template <class T>
	deque<T>::deque(const T* source, size_type count)
	{
		append_all(detail::position_range<const T*>{ source, source + count });
	}

	template <class T>
	deque<T>::deque(const deque& other)
	{
		append_all(detail::position_range<const_iterator>{ other.begin(), other.end() });
	}

	template <class T>
	deque<T>::deque(deque&& other) noexcept
	{
		steal(other);
	}

	//------------------------------------------------
	// Modifier methods.

	template <class T>
	deque<T>& deque<T>::operator=(const deque& other)
	{
		deque copy(other);
		swap(copy);
		return *this;
	}

	template <class T>
	deque<T>& deque<T>::operator=(deque&& other) noexcept
	{
		if (this == &other)
			return *this;

		clear();
		steal(other);
		return *this;
	}

	template <class T>
	deque<T>& deque<T>::clear() noexcept
	{
		while (head != nullptr)
			unlink(head);
		return *this;
	}

	//------------------------------------------------
	// Specific methods.

	template <class T>
	deque<T>& deque<T>::push_front(T value)
	{
		node_type* fresh = new node_type(std::move(value));
		fresh->next = head;
		if (head != nullptr)
			head->previous = fresh;
		else
			tail = fresh;

		head = fresh;
		++length;
		return *this;
	}

	template <class T>
	deque<T>& deque<T>::push_back(T value)
	{
		node_type* fresh = new node_type(std::move(value));
		fresh->previous = tail;
		if (tail != nullptr)
			tail->next = fresh;
		else
			head = fresh;

		tail = fresh;
		++length;
		return *this;
	}

	template <class T>
	deque<T>& deque<T>::pop_front()
	{
		if (empty())
			throw empty_error("deque::pop_front");
		unlink(head);
		return *this;
	}

	template <class T>
	deque<T>& deque<T>::pop_back()
	{
		if (empty())
			throw empty_error("deque::pop_back");
		unlink(tail);
		return *this;
	}

	//------------------------------------------------
	// Constant methods.

	template <class T>
	bool deque<T>::operator==(const deque& other) const
	{
		if (length != other.length)
			return false;

		for (const node_type *mine = head, *theirs = other.head; mine != nullptr; mine = mine->next, theirs = theirs->next)
			if (!(mine->get() == theirs->get()))
				return false;
		return true;
	}

	template <class T>
	bool deque<T>::operator!=(const deque& other) const
	{
		return !(*this == other);
	}

	template <class T>
	typename deque<T>::size_type deque<T>::size() const noexcept
	{
		return length;
	}

	template <class T>
	bool deque<T>::empty() const noexcept
	{
		return length == 0;
	}

	template <class T>
	typename deque<T>::const_reference deque<T>::front() const
	{
		if (empty())
			throw empty_error("deque::front");
		return head->get();
	}

	template <class T>
	typename deque<T>::const_reference deque<T>::back() const
	{
		if (empty())
			throw empty_error("deque::back");
		return tail->get();
	}

	//------------------------------------------------
	// Iterator methods.

	template <class T>
	typename deque<T>::iterator deque<T>::begin() noexcept
	{
		return iterator(head);
	}

	template <class T>
	typename deque<T>::iterator deque<T>::end() noexcept
	{
		return iterator(nullptr);
	}

	template <class T>
	typename deque<T>::const_iterator deque<T>::begin() const noexcept
	{
		return const_iterator(head);
	}

	template <class T>
	typename deque<T>::const_iterator deque<T>::end() const noexcept
	{
		return const_iterator(nullptr);
	}

	//------------------------------------------------
	// Private helpers.

	// Appends copies of the source; on failure the partially built chain is released before rethrowing.
	template <class T>
	template <class position>
	void deque<T>::append_all(detail::position_range<position> source)
	{
		try
		{
			for (position current = source.first; current != source.last; ++current)
				push_back(*current);
		}
		catch (...)
		{
			clear();
			throw;
		}
	}

	template <class T>
	void deque<T>::unlink(node_type* removed) noexcept
	{
		if (removed->previous != nullptr)
			removed->previous->next = removed->next;
		else
			head = removed->next;

		if (removed->next != nullptr)
			removed->next->previous = removed->previous;
		else
			tail = removed->previous;

		delete removed;
		--length;
	}

	template <class T>
	void deque<T>::steal(deque& other) noexcept
	{
		head = std::exchange(other.head, nullptr);
		tail = std::exchange(other.tail, nullptr);
		length = std::exchange(other.length, 0);
	}

	template <class T>
	void deque<T>::swap(deque& other) noexcept
	{
		using std::swap;
		swap(head, other.head);
		swap(tail, other.tail);
		swap(length, other.length);
	}
}
