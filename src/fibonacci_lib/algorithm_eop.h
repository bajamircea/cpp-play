#pragma once

#include <array>

namespace fibonacci { namespace algorithm_eop
{
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
struct fibonacci_matrix_multiply
{
  std::array<N, 2> operator()(const std::array<N, 2> & x, const std::array<N, 2> & y)
  {
    return { x[0] * (y[1] + y[0]) + x[1] * y[0],
             x[0] * y[0] + x[1] * y[1]};
   }
  };

  template <typename N> std::array<N, 2> identity_element(const fibonacci_matrix_multiply<N>&)
  {
    return { N(0), N(1) };
  }

  template <typename R, typename N> R fibonacci_pow(N n)
  {
    if (n == 0) return R(0);
    return power(std::array<R, 2>{ R(1), R(0) }, n, fibonacci_matrix_multiply<R>())[0];
  }
}} // namespace fibonacci::eop