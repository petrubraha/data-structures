# Data Structures

A header-only C++17 template library of fundamental data structures, built with Gradle and tested with GoogleTest. Every structure is a template on its element type and, where ordering matters, on a comparator.

## Structures

All public types live in the `ds` namespace. Include the header named after the structure.

| Structure            | Header            | Backing             |
| -------------------- | ----------------- | ------------------- |
| `array`              | `array.hpp`       | Growable buffer     |
| `linked_list`        | `linked_list.hpp` | Singly linked, tail |
| `stack`              | `stack.hpp`       | Linked list         |
| `queue`              | `queue.hpp`       | Linked list         |
| `deque`              | `deque.hpp`       | Doubly linked       |
| `heap<max, T>`       | `heap.hpp`        | Array, max on top   |
| `heap<min, T>`       | `heap.hpp`        | Array, min on top   |
| `tree<avl, T>`       | `tree.hpp`        | Height-balanced BST |
| `tree<red_black, T>` | `tree.hpp`        | Colour-balanced BST |
| `hash_map<K, V>`     | `hash_map.hpp`    | Separate chaining   |

Every structure follows the same method vocabulary, in this order: constructors, modifiers (`operator=`, `clear`, `set_compare`), structure-specific operations (`insert`, `push`, `sort`, etc.), constant methods (`size`, `empty`, `print`, element access), queries (`search`, `minimum`, `maximum`, `predecessor`, `successor`), iteration (`begin`, `end`), and the `operator<<` and `convert` friends. A structure leaves out a family that does not apply to it.

## Usage

```cpp
#include "array.hpp"
#include "hash_map.hpp"
#include "heap.hpp"
#include "tree.hpp"

ds::array<int> numbers{ 5, 3, 9, 1 };
numbers.sort();                              // quick sort, median-of-three pivot
numbers.sort<ds::merge_sort>();              // any algorithm policy
numbers.sort<ds::quick_sort, ds::random>();  // quick sort with another pivot strategy

ds::heap<ds::min, int> smallest_first{ 5, 3, 9, 1 };
int smallest = smallest_first.extract();     // 1

ds::tree<ds::red_black, int> ordered{ 5, 3, 9, 1 };
auto three = ordered.search(3);
auto five = ordered.successor(three);        // in-order neighbour

ds::hash_map<std::string, int> ages;
ages.insert("ada", 36).insert("alan", 41);
int age = ages.get("ada");
```

Ordering structures take the comparator as a template argument, `std::less<T>` by default, so element types need no `operator<` when a comparator is given:

```cpp
struct by_length
{
	bool operator()(const std::string& left, const std::string& right) const
	{
		return left.size() < right.size();
	}
};

ds::tree<ds::avl, std::string, by_length> words{ "graph", "map", "heap" };
```

The sorting algorithms are `bubble_sort`, `selection_sort`, `insertion_sort`, `merge_sort`, `heap_sort` and `quick_sort`. The quick sort pivot strategies are `median_of_three` (default), `first`, `last`, `middle` and `random`.

## Errors

Recoverable misuse throws a type derived from `<stdexcept>`, with a message that names the operation and the values involved, e.g. `array::get: index 7 out of range, size is 5`.

| Exception        | Base                | Thrown on           |
| ---------------- | ------------------- | ------------------- |
| `index_error`    | `std::out_of_range` | Bad index           |
| `empty_error`    | `std::out_of_range` | Access when empty   |
| `key_error`      | `std::out_of_range` | Missing map key     |
| `position_error` | `std::out_of_range` | Unusable iterator   |
| `capacity_error` | `std::length_error` | Impossible capacity |

Dereferencing or advancing an end iterator is not recoverable: the process prints the reason to standard error and aborts.

## Build and Test

Requirements:

- **Toolchain:** Visual Studio 2022 with the C++ workload, x86-64.
- **Java:** JDK 17 or newer, to run the Gradle wrapper.
- **Network:** the first build downloads Gradle and GoogleTest.

```text
gradlew build    # compiles the library and runs the test suite
gradlew test     # runs the test suite only
```

The test executable runs under AddressSanitizer, and a leak check fails any test that ends with more live allocations than it started with.

## Depending on the Library

From another Gradle C++ build, include this repository as a composite build and depend on the `lib` module:

```groovy
// settings.gradle
includeBuild('../data-structures')

// build.gradle
application {
    dependencies {
        implementation 'data-structures:lib'
    }
}
```

Without Gradle, add `lib/src/main/public` to the include path and compile with C++17. Nothing needs to be linked.

## Layout

```text
lib/
  build.gradle
  src/main/public/          public headers, one per structure
  src/main/public/detail/   internal headers (ds::detail): nodes, iterators, policies, sorting, utilities
  src/main/cpp/library.cpp  compiles every public header once
  src/test/cpp/             GoogleTest suites and test support
docs/theory.md              notes on the algorithms and design choices
```
