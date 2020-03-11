#pragma once

#include "big_number.h"

#include <array>
#include <cstdint>

namespace fibonacci { namespace algorithm
{
  std::uint64_t exponential_naive(std::uint32_t value);
  std::uint64_t linear_naive(std::uint32_t value);
  fibonacci::big_number::unsigned_binary linear_big(std::uint32_t value);
  std::uint32_t linear_first_big();

  // from https://sean-parent.stlab.cc/presentations/2017-01-18-concurrency/2017-01-18-concurrency.pdf
  template <typename T, typename N, typename O>
  T power(T x, N n, O op)
  {
    if (n == 0) return identity_element(op);
    while ((n & 1) == 0)
    {
      n >>= 1;
      x = op(x, x);
    }
    T result = x;
    n >>= 1;
    while (n != 0)
    {
      x = op(x, x);
      if ((n & 1) != 0)
      {
        result = op(result, x);
      }
      n >>= 1;
    }
    return result;
  }

template <typename N>
struct multiply_2x2
{
  std::array<N, 4> operator()(const std::array<N, 4> & x, const std::array<N, 4> & y)
  {
    return { x[0] * y[0] + x[1] * y[2], x[0] * y[1] + x[1] * y[3],
             x[2] * y[0] + x[3] * y[2], x[2] * y[1] + x[3] * y[3] };
   }
  };
  
  template <typename N> std::array<N, 4> identity_element(const multiply_2x2<N>&)
  {
    return { N(1), N(0), N(0), N(1) };
  }
  
  template <typename R, typename N> R fibonacci_pow(N n)
  {
    if (n == 0) return R(0);
    return power(std::array<R, 4>{ R(1), R(1), R(1), R(0) }, N(n - 1), multiply_2x2<R>())[0];
  }
}} // namespace fibonacci::algorithm