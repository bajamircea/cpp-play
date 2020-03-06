#pragma once

#include <cstddef>

namespace fibonacci
{
  template<typename T>
  // requires(Integer(T))
  T recurse(T x)
  {
    // Precondition: x >= 0
    if (x == 0) return 0;
    if (x == 1) return 1;
    return recurse(x - 1) + recurse(x - 2);
  }

  template<typename T>
  // requires(Integer(T))
  T linear(T x)
  {
    // Precondition: x >= 0
    if (x == 0) return 0;
    T crt[2] = { 0, 1 };
    for (T i = 1; i < x ; ++i)
    {
      T sum = crt[0] + crt[1];
      crt[0] = crt[1];
      crt[1] = sum;
    }
    return crt[1];
  }
} // namespace fibonacci
