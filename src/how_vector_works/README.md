# How vector works

Use instrumented types that count the number of constructor/destructor/copy/move operations, with variations around `noexcept`, to get insights into `std::vector` implementation when resizing e.g. growth factor and move vs. copy of elements.

The output of the instrumentation counts things like:
- mc = move constructor
- cc = copy constructor


## Project structure

Single `main.cpp` file.

Tested with g++ on Ubuntu, and Microsoft Visual Studio on Windows. Strictly speaking, the behaviour relates to the standard libraries implementations, not to the compilers themselves.


## Allocation steps

Tests the allocation steps.

E.g. output below indicatates that `std::vector` uses a growth factor of 2x when capacity is reached (g++), i.e. capacity grows 1, 2, 4, 8, 16 ...:

```
==== test for allocation steps
o: 1, mc: 1, ~: 1
size: 1, capacity: 1
o: 1, mc: 2, ~: 2
size: 2, capacity: 2
o: 1, mc: 3, ~: 3
size: 3, capacity: 4
o: 1, mc: 1, ~: 1
size: 4, capacity: 4
o: 1, mc: 5, ~: 5
size: 5, capacity: 8
o: 1, mc: 1, ~: 1
size: 6, capacity: 8
```

E.g. growth factor of 1.5x when capacity is reached (Microsoft) , i.e. capacity grows 1, 2, 3, 4, 6, 9, 13 ...:
```
==== test for allocation steps
o: 1, mc: 1, ~: 1
size: 1, capacity: 1
o: 1, mc: 2, ~: 2
size: 2, capacity: 2
o: 1, mc: 3, ~: 3
size: 3, capacity: 3
o: 1, mc: 4, ~: 4
size: 4, capacity: 4
o: 1, mc: 5, ~: 5
size: 5, capacity: 6
o: 1, mc: 1, ~: 1
size: 6, capacity: 6
o: 1, mc: 7, ~: 7
size: 7, capacity: 9
o: 1, mc: 1, ~: 1
size: 8, capacity: 9
o: 1, mc: 1, ~: 1
size: 9, capacity: 9
o: 1, mc: 10, ~: 10
size: 10, capacity: 13
```


## Fill test

For a container of `n` entries, test the average excess memory usage (depends on growth factor)

E.g. for growth factor of 2x, with `n = 1024`:
```
==== fill test
average waste: 0.381265
```

for growth factor of 1.5x, with `n = 1066`::
```
==== fill test
average waste: 0.211812
```


## Copy vs. move

Tests the operation used when resizing.

On resize the simple type is moved:
```
==== simple type
o: 1, mc: 1, ~: 1
size: 1, capacity: 1
o: 1, mc: 2, ~: 2
size: 2, capacity: 2
o: 1, mc: 3, ~: 3
size: 3, capacity: 4
o: 1, mc: 1, ~: 1
size: 4, capacity: 4
```

On resize the copy only type is copied:
```
==== copy only
o: 1, cc: 1, ~: 1
size: 1, capacity: 1
o: 1, cc: 2, ~: 2
size: 2, capacity: 2
o: 1, cc: 3, ~: 3
size: 3, capacity: 4
o: 1, cc: 1, ~: 1
size: 4, capacity: 4
```

As a baseline, the move is done if for the value type T:
- `is_nothrow_move_constructible<T>`
- OR `negation<is_copy_constructible<Ty>>`
See
- `__move_if_noexcept_cond` for g++
- `_Umove_if_noexcept1` for Microsoft

But the copy only class is more weird than at first look because it turns out that `std::is_nothrow_move_constructible_v<instrumented_copy_only>` is true. That comes to "Types without a move constructor, but with a copy constructor that accepts const T& arguments, satisfy std::is_move_constructible". Despite the type traits saying that it's movable, it's copied actually instead.

A simple container that move constructs `noexcept` is moved on resize, just like a simple type:
```
==== simple container
o: 1, mc: 1, ~: 1
size: 1, capacity: 1
o: 1, mc: 2, ~: 2
size: 2, capacity: 2
o: 1, mc: 3, ~: 3
size: 3, capacity: 4
o: 1, mc: 1, ~: 1
size: 4, capacity: 4
```

RAII types that move, but don't copy, are also moved:
```
==== raii
o: 1, mc: 1, ~: 1
size: 1, capacity: 1
o: 1, mc: 2, ~: 2
size: 2, capacity: 2
o: 1, mc: 3, ~: 3
size: 3, capacity: 4
o: 1, mc: 1, ~: 1
size: 4, capacity: 4
```
Even if the move constructor for RAII class throws, the values will still be move constructed, but the `push_back` only gives basic exception guarantees instead of the strong ones. 


# Containers with dynamically allocated sentinels

If the move constructor throws, currently g++ and Microsoft copy construct when resizing:
```
==== container with dynamically allocated sentinel
o: 1, mc: 1, ~: 1
size: 1, capacity: 1
o: 1, cc: 1, mc: 1, ~: 2
size: 2, capacity: 2
o: 1, cc: 2, mc: 1, ~: 3
size: 3, capacity: 3
o: 1, cc: 3, mc: 1, ~: 4
size: 4, capacity: 4
o: 1, cc: 4, mc: 1, ~: 5
size: 5, capacity: 6
o: 1, mc: 1, ~: 1
size: 6, capacity: 6
```

In g++ `std::list` is moved on resize (hence constant `cc: 1`):
```
==== list of simple type
o: 1, cc: 1, ~: 1
size: 1, capacity: 1
o: 1, cc: 1, ~: 1
size: 2, capacity: 2
o: 1, cc: 1, ~: 1
size: 3, capacity: 4
o: 1, cc: 1, ~: 1
size: 4, capacity: 4
o: 1, cc: 1, ~: 1
size: 5, capacity: 8
o: 1, cc: 1, ~: 1
size: 6, capacity: 8
```

But the Microsoft `std::list` implementation uses dynamically allocated sentinels, and therefore is copied (list values are copy constructed on resize):
```
==== list of simple type
o: 1, cc: 1, ~: 1
size: 1, capacity: 1
o: 1, cc: 2, ~: 2
size: 2, capacity: 2
o: 1, cc: 3, ~: 3
size: 3, capacity: 3
o: 1, cc: 4, ~: 4
size: 4, capacity: 4
o: 1, cc: 5, ~: 5
size: 5, capacity: 6
o: 1, cc: 1, ~: 1
size: 6, capacity: 6
```


## Container proding

As far as the instrumented types, and `std::vector<int>` both compilers confirm:
```
instrumented_simple:
  is_nothrow_default_constructible:yes
  is_copy_constructible:           yes
  is_nothrow_move_constructible:   yes
instrumented_copy_only:
  is_nothrow_default_constructible:yes
  is_copy_constructible:           yes
  is_nothrow_move_constructible:   yes
instrumented_container:
  is_nothrow_default_constructible:yes
  is_copy_constructible:           yes
  is_nothrow_move_constructible:   yes
instrumented_raii:
  is_nothrow_default_constructible:no
  is_copy_constructible:           no
  is_nothrow_move_constructible:   yes
instrumented_sentinel:
  is_nothrow_default_constructible:no
  is_copy_constructible:           yes
  is_nothrow_move_constructible:   no
std::vector<int>:
  is_nothrow_default_constructible:yes
  is_copy_constructible:           yes
  is_nothrow_move_constructible:   yes
```

But while `std::list` in g++:
```
std::list<int>:
  is_nothrow_default_constructible:yes
  is_copy_constructible:           yes
  is_nothrow_move_constructible:   yes
```

the Microsoft `std::list` behaves likes `instrumented_sentinel` (hence the copy of values we've seen above):
```
std::list<int>:
  is_nothrow_default_constructible:no
  is_copy_constructible:           yes
  is_nothrow_move_constructible:   no
```

Microsoft took the same sentinal approach for:
- `std::list`:
- `std::map`
- `std::set`
- `std::unordered_map`
- `std::deque`
while `std::string` is like `std::vector`


g++ does something special about deque, because you'd expect it to be sentinel-like:
```
std::deque<int>:
  is_nothrow_default_constructible:no
  is_copy_constructible:           yes
  is_nothrow_move_constructible:   no
```

But it moves it when resizing a vector of deque:
```
==== deque of simple type
o: 1, cc: 1, ~: 1
size: 1, capacity: 1
o: 1, cc: 1, ~: 1
size: 2, capacity: 2
o: 1, cc: 1, ~: 1
size: 3, capacity: 4
o: 1, cc: 1, ~: 1
size: 4, capacity: 4
o: 1, cc: 1, ~: 1
size: 5, capacity: 8
o: 1, cc: 1, ~: 1
size: 6, capacity: 8
```

TODO:
read g++ vector source code, understand the g++ deque case

read https://stackoverflow.com/questions/17730689/is-a-moved-from-vector-always-empty
in particular, figure out how empty is a "moved from" vector, list etc.

https://isocpp.org/files/papers/N4055.html