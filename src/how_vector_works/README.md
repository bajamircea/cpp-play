# How vector works

Use instrumented types that count the number of constructor/destructor/copy/move operations, with variations around `noexcept`, to get insights into `std::vector` implementation when resizing e.g. growth factor and move vs. copy of elements.

The output of the instrumentation counts things like:
- mc = move constructor
- cc = copy constructor


## Project structure

Single `main.cpp` file.

Tested with GCC/g++ on Ubuntu, and Microsoft Visual Studio on Windows.


## Allocation steps

Tests the allocation steps.

E.g. output below indicatates that `std::vector` uses a growth factor of 2x when capacity is reached (gcc), i.e. capacity grows 1, 2, 4, 8, 16 ...:

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


## Copy vs. move

Tests the operation used when resizing, e.g. move constructor for the simple type, and copy constructor for the copy-only type:

```
==== simple type
o: 1, mc: 1, ~: 1
size: 1, capacity: 1
o: 1, mc: 2, ~: 2
size: 2, capacity: 2
o: 1, mc: 3, ~: 3
size: 3, capacity: 4
...
==== copy-only type
o: 1, cc: 1, ~: 1
size: 1, capacity: 1
o: 1, cc: 2, ~: 2
size: 2, capacity: 2
o: 1, cc: 3, ~: 3
size: 3, capacity: 4
```

The move is done if for the value type T:
- `is_nothrow_move_constructible<T>`
- OR `negation<is_copy_constructible<Ty>>`
See
- `__move_if_noexcept_cond` for GCC
- `_Umove_if_noexcept1` for Microsoft


# Containers with dynamically allocated sentinels

The move constructor throws, currently GCC and Microsoft copy construct when resizing:
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


In GCC std::list is moved on resize:
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

But the Microsoft std::list is copied (list values are copy constructed on resize):
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


## Fill test

For a container of `n` entries, test the average excess memory usage (depends on growth factor)

E.g. for growth factor of 2x:
```
==== fill test
average waste: 0.381265
```

for growth factor of 1.5x:
```
==== fill test
average waste: 0.211812
```

## Container proding

In GCC:

```
==== container operations
std::vector<int>:
  is_copy_constructible:         yes
  is_nothrow_move_constructible: yes
  is_nothrow_move_assignable:    yes
std::list<int>:
  is_copy_constructible:         yes
  is_nothrow_move_constructible: yes
  is_nothrow_move_assignable:    yes
std::vector<std::list<int>>:
  is_copy_constructible:         yes
  is_nothrow_move_constructible: yes
  is_nothrow_move_assignable:    yes
std::map<int, int>:
  is_copy_constructible:         yes
  is_nothrow_move_constructible: yes
  is_nothrow_move_assignable:    yes
std::set<int>:
  is_copy_constructible:         yes
  is_nothrow_move_constructible: yes
  is_nothrow_move_assignable:    yes
std::unordered_map<int, int>:
  is_copy_constructible:         yes
  is_nothrow_move_constructible: yes
  is_nothrow_move_assignable:    yes
std::deque<int>:
  is_copy_constructible:         yes
  is_nothrow_move_constructible: no
  is_nothrow_move_assignable:    yes
std::string:
  is_copy_constructible:         yes
  is_nothrow_move_constructible: yes
  is_nothrow_move_assignable:    yes
```

TODO: in Microsoft
TODO: why deque in GCC