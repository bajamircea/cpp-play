# Fibonacci

This is a play on Fibonacci implementations, see comments below.

## Project structure

- `src/fibonacci_lib` - code for implementing fibonacci
  - `algoritm_naive[.h|.cpp]` - naive algorithms
  - `algoritm_sean_parent.h` - Sean Parent version
  - `algoritm_eop.h` - Elements of Programming version
  -`big_number[.h|.cpp]` - support for big numbers
- `src/fibonacci` - the main program with various argument options using code from `fibonacci_lib`
- `src/fibonacci_lib_test` - tests for `fibonacci_lib`

## Naive implementations

Obviously naive implementations of Fibonacci are slow. Everyone and his dog should be able to implement
it recursively, but that has exponential complexity and quickly takes seconds for mid double digit arguments.

With a bit of care, all programmers should be able to implement linear complexity versions. 


## Elements of Programming chapter 3

Elements of Programming chapter 3 is a beatiful chapter with multiple gems. Mainly it works through
implementing an algorithm to calculate "power of" in `O(log(N))`. This fits in the general theme
of the book of ilustrating the importance of mathematical properties for algorithms, particularly
associativity here. It also shows how to optimise the algorithm, starting with recursion elimination.

At the end of the chapter 3 it shows how the "power of" can be used to calculate the Fibonacci function
in what looks like `O(log(N))`.


## Sean Parent

In the middle of his [Better Code: Concurrency](https://sean-parent.stlab.cc/presentations/2017-01-18-concurrency/2017-01-18-concurrency.pdf),
Sean Parent mentions that Fibonacci is often used as an example of a slow function in parallel
algorithms, then he proceeds to show how to do it properly and calculate Fibonacci for 1 million,
a result of 208,988 digits in 0.72 seconds. (FYI: The results starts in `1953282128707757731632`
and ends in `96526838242546875`).

He uses `boost::multiprecision::cpp_int` as the underling large integer data structure.

Note: There is the choice of starting the Fibonacci series with a 0 or with an 1. Sean Parent and Elements of
Programming use the version starting at 0.

## Really?

So I wondered, for fun, what would it take to play with it? The plan is to not use boost, but instead
to write a `big_number::unsigned_binary` data structure that uses a vector of 32 bit unsigned integers,
value at index 0 is the least significant. An empty vector would represent zero.

To save on development effort, I did not make `big_number::unsigned_binary` a template on the type
used in the vector. The disadvantage is that it's hard to substitute with mocks to count the number
of operations. For the short term it does save effort.

Then I planned to pair with a data structure that stores the decimal representation that could easily
be printed. I started with `big_number::unsigned_decimal` using a vector of `char`s, value at index 0
is the least significant, where each value is between `0` and `9` inclusive. I could easily convert
this to/from string by adding/substracting `'0'` (which is `48` in ASCII).

With that it took about **2 seconds to calculate** Fibonacci in a `unsigned_binary`, but a
**very long time to convert** to `unsigned_decimal` (and print the decimal result).


## Improve conversion to decimal

So then I changed the approach used for `unsigned_decimal` and stored instead a vector of 32 bit unsigned
integers in a vector, where each value represents a number in base 1,000,000,000 (from `0` to `999,999,999`).

It also turns out that the functions `from_chars` and `to_chars` from the `<charconv>` header might not be
useful in this case, as all the values from the vector (except the one at the highest index) need to be
padded with `0`s.

In this approach I used a lot of info that's not impossible, but not trivial to deduce programatically either.
E.g. the fact that base 1,000,000,000 is the best fit for a power of 10 into a 32 bit unsigned integer. Also
used switches up to `9` to calculate the lenght of the serialized string.

With that it took **about 1 second to convert** to `unsigned_decimal` (and print the decimal result, though
most of the time is taken by the conversion).


## Improving operations

First the [recommended way to write arithmetic operators](https://en.cppreference.com/w/cpp/language/operators)
is a bit weird.
```cpp
  friend T operator+(T lhs, const T & rhs);
  // delegating to:
  T & operator+=(const T & rhs);
```
The idea is to optimise chains of the form `a + b + c`. How much this makes a difference here ... am not sure.
But notice that doing `a + b` involves creating a temporary, which involves an allocation for the vector in
our big integers.

Addition is `O(N)`, multiplication on the other side when implemented as learned in school (long multiplication)
is `O(N^2)`. There are algorithms that do a bit better, one such is the Karatsuba algorithm that has a complexity
of about `O(N^1.58)`. It might not seem much of a difference but compare `N = 10`: `10^2 = 100`, while
`10^1.58 ~= 38`, which is less the half.

Unfortunately it also comes with constants due to additional allocations (unless one is willing to go into custom
allocators territory), so empirically it seems that the threshold for using Karatsuba vs. long multiplication
is around `100` for the size of vectors of the operands.

With that it took **about 0.5 seconds to calculate Fibonacci, plus about 1 second** to convert
and print.


## Elements of Programming chapter 3 - revisited

It turns out that Sean Parent's example is from "From Mathematics to Generic Programming", not from
Elements of Programming.

In Elements of Programming the implementation uses half of matrix. Using this approach takes it to
**about 0.3 seconds to calculate Fibonacci, plus about 1 second** to convert
and print. So not the calculation is dominated by the conversion (and print).

I stopped there. Could have tried to see what difference it makes by using a more optimised power algorith.
Could also spend more effort on the conversion issue.

Interestingly the power in EOP uses same type for argument and return value, I used different types: unsigned 32 bit
for the argument of 1 million and `unsigned_binary` for the return value.
