#pragma once
#include "node.hpp"

namespace ds::detail
{
	// Singly linked node.
	template <class T>
	class list_node : public node<T>
	{
	public:
		explicit list_node(T initial) : node<T>(std::move(initial)) {}

		list_node* next = nullptr;
	};
}
