#pragma once
#include "node.hpp"

namespace ds::detail
{
	// Doubly linked node.
	template <class T>
	class deque_node : public node<T>
	{
	public:
		explicit deque_node(T initial) : node<T>(std::move(initial)) {}

		deque_node* previous = nullptr;
		deque_node* next = nullptr;
	};
}
