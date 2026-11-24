#pragma once
#include <utility>

namespace ds::detail
{
	// Value holder shared by every linked node. Construction is protected, so only a concrete node exists.
	template <class T>
	class node
	{
	public:
		node(const node&) = delete;
		node& operator=(const node&) = delete;

		T& get() noexcept;
		const T& get() const noexcept;
		void set(T replacement);

	protected:
		explicit node(T initial);
		~node() = default;

	private:
		T value;
	};

	//------------------------------------------------

	template <class T>
	node<T>::node(T initial) : value(std::move(initial)) {}

	template <class T>
	T& node<T>::get() noexcept
	{
		return value;
	}

	template <class T>
	const T& node<T>::get() const noexcept
	{
		return value;
	}

	template <class T>
	void node<T>::set(T replacement)
	{
		value = std::move(replacement);
	}
}
