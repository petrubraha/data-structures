#pragma once
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace ds
{
	// Thrown when a position lies outside the stored elements.
	class index_error : public std::out_of_range
	{
	public:
		index_error(const std::string& operation, std::size_t index, std::size_t size)
			: std::out_of_range(operation + ": index " + std::to_string(index)
				+ " out of range, size is " + std::to_string(size)) {}
	};

	// Thrown when an operation needs an element and the structure holds none.
	class empty_error : public std::out_of_range
	{
	public:
		explicit empty_error(const std::string& operation)
			: std::out_of_range(operation + ": no element to access, structure is empty") {}
	};

	// Thrown when a lookup asks for a key the structure does not hold.
	class key_error : public std::out_of_range
	{
	public:
		explicit key_error(const std::string& operation)
			: std::out_of_range(operation + ": key not found") {}
	};

	// Thrown when an iterator position cannot serve the operation, such as inserting after the end.
	class position_error : public std::out_of_range
	{
	public:
		position_error(const std::string& operation, const std::string& reason)
			: std::out_of_range(operation + ": " + reason) {}
	};

	// Thrown when a requested capacity exceeds what the structure can allocate.
	class capacity_error : public std::length_error
	{
	public:
		capacity_error(const std::string& operation, std::size_t requested, std::size_t maximum)
			: std::length_error(operation + ": capacity " + std::to_string(requested)
				+ " exceeds the maximum of " + std::to_string(maximum)) {}
	};

	namespace detail
	{
		// The single terminate path, for states where continuing would be undefined behaviour.
		[[noreturn]] inline void fail_fast(const char* message) noexcept
		{
			std::fputs(message, stderr);
			std::fputc('\n', stderr);
			std::abort();
		}
	}
}
