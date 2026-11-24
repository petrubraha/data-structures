# Theory Notes

Background for the design choices in the library.

## SOLID Principles

- **Single responsibility:** each class or function has one job.
- **Open-closed:** extend behaviour without editing existing code.
- **Liskov substitution:** a subtype works wherever its base does.
- **Interface segregation:** no type depends on methods it does not use.
- **Dependency inversion:** high-level code depends on abstractions, not details.

The library applies open-closed through policies: a new sorting algorithm, pivot strategy, heap order or balancing scheme is a new tag type, and the structures that use it stay unchanged.

## Converting a Structure to Memory

A structure is easiest to combine with others when it can hand out a copy of its data in a plain form. The shape of that copy should be one level down, not the most primitive form: a stack of queues of ints hands out its queues, not a flat block of ints. Each iterable structure therefore has a `convert` friend that returns a newly allocated block holding a copy of its elements in traversal order.

## Quick Sort

Pivot strategies, all available as `quick_sort` policies:

- **First, last:** cheapest, but quadratic on sorted or reversed input.
- **Middle:** cheap, and safe on sorted or reversed input.
- **Median of three:** the default, robust on common patterns.
- **Random:** expected O(n log n) on every input.

Partition schemes:

- **Lomuto:** simpler, more swaps, degrades on many duplicates.
- **Hoare:** fewer swaps, scans from both ends; the library uses it.

The library moves the chosen pivot to the front before partitioning, so Hoare's split never lands on the last index whatever the strategy. It recurses into the smaller part and loops over the larger one, which bounds the stack depth to O(log n).

| Measure     | Quick sort | Merge sort | Heap sort  |
| ----------- | ---------- | ---------- | ---------- |
| Worst time  | O(n^2)     | O(n log n) | O(n log n) |
| Expected    | O(n log n) | O(n log n) | O(n log n) |
| Extra space | O(log n)   | O(n)       | O(1)       |
| Stable      | No         | Yes        | No         |

Quick sort is usually the fastest of the three in practice because its inner loop is short and works in place. Merge sort is the choice when equal elements must keep their order; heap sort when no extra memory is allowed.

## Heaps

A binary heap stored level by level in an array needs no links: the parent of position i is (i - 1) / 2 and its children are 2i + 1 and 2i + 2. Building a heap from n elements by sifting down from the last parent takes O(n), less than n separate insertions. The max and min heaps differ only in which way the comparator is read, so one implementation serves both.

## Balanced Trees

An unbalanced binary search tree degrades to a list on sorted input. Both balancing schemes keep the height logarithmic:

- **AVL:** subtree heights differ by at most one; stricter, faster lookups.
- **Red-black:** colour rules bound height to 2 log n; fewer rotations on update.

Insertion and removal are the same binary search tree operations in both; only the repair step afterwards differs, which is why the balancing scheme is a policy of a single tree.
