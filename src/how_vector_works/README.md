# How vector works

The idea is to use instrumented types that count the number of constructor/destructor/copy/move operations
to get insights into `std::vector` implementation metrics

## Project structure

- single `main.cpp` file

## Allocation steps

Tests the allocation steps, e.g. output below indicatates that `std::vector` doubles size
when capacity is reached:

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

NOTE: of particular interest is the behaviour for types that throw in move constructors
used by containers with dynamically allocated sentinels.


## Fill test

For a container of about 1 million entries, test the average usage of capacity. E.g. about 75%
in the example below.

```
==== fill test
average usage: 0.750005
```
