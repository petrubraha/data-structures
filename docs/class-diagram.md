# Class Diagrams

Three diagrams. The first shows the public data structures and the interface each one exposes. The second shows the supporting classes (nodes, iterators) and the policy tags, with the inheritance-versus-composition decision recorded for every relationship. The third shows the sorting policies behind `array::sort`.

Public types live in the `ds` namespace under `lib/src/main/public/`. Supporting types live in `ds::detail` under `lib/src/main/public/detail/` and are never included directly by a consumer; the public headers pull them in. The policy tags (`max`, `min`, `avl`, `red_black`, the sorting algorithms and pivot strategies) and the exception types are in `ds`, because consumers name them, even though their headers are internal.

## Diagram 1: data structures and their interfaces

Each structure is an independent template. They share a method vocabulary by convention, not by a common base class (see Diagram 2 for why). A structure lists only the method families that apply to it. Every structure also has the rule of five (destructor, default, initializer-list, raw-buffer, copy and move constructors, copy and move assignment).

```mermaid
classDiagram
    direction LR

    class array~T, compare~ {
        +clear() +set_compare()
        +insert(value) +insert(value, index) +remove(index) +sort~algorithm, options~()
        +operator==() +size() +capacity() +empty() +print() +get() +operator[]()
        +search() +minimum() +maximum() +predecessor() +successor()
        +begin() +end()
        +operator<<() +convert()
        +npos$ +default_capacity$
    }
    class linked_list~T, compare~ {
        +clear() +set_compare()
        +insert(value) +insert(value, index) +insert_after() +remove(index) +remove_after()
        +operator==() +size() +empty() +print() +front() +back()
        +search() +minimum() +maximum() +predecessor() +successor()
        +begin() +end()
        +operator<<() +convert()
    }
    class stack~T~ {
        +push() +pop()
        +operator==() +size() +empty() +top()
    }
    class queue~T~ {
        +push() +pop()
        +operator==() +size() +empty() +front() +back()
    }
    class deque~T~ {
        +clear()
        +push_front() +push_back() +pop_front() +pop_back()
        +operator==() +size() +empty() +front() +back()
        +begin() +end()
        +operator<<() +convert()
    }
    class heap~order, T, compare~ {
        +clear() +set_compare()
        +insert() +extract() +replace()
        +operator==() +size() +empty() +print() +peek()
        +search()
        +begin() +end()
        +operator<<() +convert()
        +npos$
    }
    class tree~balancing, T, compare~ {
        +clear() +set_compare()
        +insert() +remove()
        +operator==() +size() +empty() +print()
        +search() +minimum() +maximum() +predecessor() +successor()
        +begin() +end()
        +operator<<() +convert()
    }
    class hash_map~key_type, mapped_type, hasher~ {
        +clear()
        +insert() +remove()
        +operator==() +size() +empty() +print() +get() +contains()
        +begin() +end()
        +operator<<() +convert()
    }
```

## Diagram 2: supporting classes and policies

Nodes, iterators, and the compile-time policy tags, with the relationship to each structure.

```mermaid
classDiagram
    direction LR

    class node~T~ {
        <<abstract>>
        -T value
        +get() +set()
        #node(value)
    }
    class list_node~T~ {
        +list_node* next
    }
    class deque_node~T~ {
        +deque_node* previous
        +deque_node* next
    }
    class tree_node~T, augmentation~ {
        +tree_node* parent
        +child(side)
        +augmentation augment
    }

    class array_iterator~T~ {
        +operator*() +operator++() +operator!=()
    }
    class list_iterator~node_type, T~ {
        +operator*() +operator++() +operator!=()
    }
    class tree_iterator~node_type, T~ {
        +operator*() +operator++() +operator!=()
    }
    class hash_iterator~node_type, T~ {
        +operator*() +operator++() +operator!=()
    }

    class max {
        +sift_order()$
    }
    class min {
        +sift_order()$
    }
    class avl {
        +augmentation = height
        +after_insert()$ +after_remove()$
    }
    class red_black {
        +augmentation = colour
        +after_insert()$ +after_remove()$
    }

    list_node --|> node : is-a
    deque_node --|> node : is-a
    tree_node --|> node : is-a

    linked_list ..> list_node : owns
    hash_map ..> list_node : owns
    deque ..> deque_node : owns
    tree ..> tree_node : owns

    stack *-- linked_list : has-a
    queue *-- linked_list : has-a
    heap *-- array : has-a

    array ..> array_iterator : yields
    heap ..> array_iterator : yields
    linked_list ..> list_iterator : yields
    deque ..> list_iterator : yields
    tree ..> tree_iterator : yields
    hash_map ..> hash_iterator : yields

    heap ..> max : tag
    heap ..> min : tag
    tree ..> avl : tag
    tree ..> red_black : tag
```

## Diagram 3: sorting policies

`array::sort<algorithm, options...>()` calls `algorithm::apply<options...>(values, size, comparator)`. Each tag forwards to a `ds::detail` function template over positional access, one algorithm per header in `sorting/`. Only `quick_sort` takes an option, the pivot strategy.

```mermaid
classDiagram
    direction LR

    class bubble_sort { +apply()$ }
    class selection_sort { +apply()$ }
    class insertion_sort { +apply()$ }
    class merge_sort { +apply()$ }
    class heap_sort { +apply()$ }
    class quick_sort { +apply~pivot~()$ }

    class median_of_three { +operator()() }
    class first { +operator()() }
    class last { +operator()() }
    class middle { +operator()() }
    class random { -minstd_rand generator +operator()() }

    array ..> bubble_sort : sort policy
    array ..> selection_sort : sort policy
    array ..> insertion_sort : sort policy
    array ..> merge_sort : sort policy
    array ..> heap_sort : sort policy
    array ..> quick_sort : sort policy (default)

    quick_sort ..> median_of_three : pivot (default)
    quick_sort ..> first : pivot
    quick_sort ..> last : pivot
    quick_sort ..> middle : pivot
    quick_sort ..> random : pivot
    heap ..> heap_sort : reuses build_heap, sift_down
```

## Design decisions: inheritance vs composition

The original code expressed almost everything through inheritance, including a six-way abstract base (`data_structure`) and empty placeholder bases (`query_operations`, `specific_methods`). Each relationship is re-evaluated below. The rule applied: inheritance only for a genuine is-a that removes real duplication and is never on a hot copy path; composition otherwise; abstract base only when the shared code is behavior, not a name list.

### Nodes: inheritance (abstract base `node`)

- **Decision:** keep `node<T>` as an abstract base; `list_node`, `deque_node`, `tree_node` derive from it.
- **Why:** every node is-a value holder with links. The value field, `get`, and `set` are identical across all three; a base removes that duplication once. The subtypes differ only in link count and shape, which is exactly what subclassing expresses.
- **Abstract without a vtable:** the constructor and destructor are protected and non-virtual, so `node<T>` cannot be created on its own and nodes carry no vtable. The subtype is always known at the call site.

### Iterators: a shared concept, no inheritance

- **Decision:** no iterator base. Each iterator is a standalone type that satisfies the same informal concept: `operator*`, `operator++`, `operator!=`. A mutable iterator converts to its read-only counterpart.
- **Why:** iterators are copied and advanced in every loop iteration. A virtual base puts a vtable on that object and makes `operator++`/`operator*` indirect calls, which contradicts the speed-first goal and blocks inlining.
- **Shared code:** the generic linear queries (`find`, `first_extreme`, `closest_before`) are written once against the concept and serve the array (through raw pointers) and the linked list (through `list_iterator`).
- **Safety:** dereferencing or advancing an end iterator goes through `detail::fail_fast`, the single terminate path.

### Structure interface: convention, not a base class

- **Decision:** remove `data_structure` and the six method-family bases. The common method vocabulary is a documented convention (Diagram 1), not inheritance.
- **Why:** the structures are unrelated templates; nothing ever holds one through a base pointer, so the virtuals bought no polymorphism and cost a vtable on every container.

### Set operations (`integrates`, `eliminates`, `intersects`): removed

- **Decision:** drop the `instance_synergy` base and its operations entirely. They are not reimplemented in any form.

### stack / queue / heap: composition

- **Decision:** `stack` and `queue` each hold a `linked_list`; `heap` holds an `array`. They delegate and default their special members.
- **Why:** a stack is-not-a list; it restricts a list. Composition exposes only the narrowed interface and hides the rest.

### Heap and tree policies: compile-time tags, not inheritance

- **Decision:** `heap<order, T>` and `tree<balancing, T>` select behavior through a tag type resolved at compile time.
- **Heap:** the tag turns the comparator into the sift ordering (`max` keeps it, `min` reverses it), and the sift and build steps are the ones heap sort uses.
- **Tree:** the tag provides the node augmentation (height for `avl`, colour for `red_black`) and the `after_insert`/`after_remove` rebalance. The core reports each removal as the replacing child, its parent and the removed augmentation, which is all either policy needs.
