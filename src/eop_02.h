#pragma once

#include <cstddef>

namespace eop_02
{

  template<typename T, typename N, typename F>
  // requires Transformation(F) && Integer(N) && Domain(F) == T
  T power_unary(T x, N n, F f)
  {
    // Precondition:
    // - n >= 0
    // - f^i(x) defined for all i where (0 <= i < n)
    while( n != N{ 0 } )
    {
      --n;
      x = f(x);
    }
    return x;
  }

  // on a 64 bit machine, as I have, size_t is enough
  using distance_type = std::size_t;

  template<typename T, typename F>
  // requires Transformation(F) && Domain(F) == T
  distance_type distance(T x, T y, F f)
  {
    // Precondition:
    // - exists i such that f^i(x) == y
    distance_type result{ 0 };
    while (x != y)
    {
      x = f(x);
      ++result;
    }
    return result;
  }

  template<typename T, typename F, typename P>
  // requires Transformation(F) && Domain(F) == T &&
  //   Predicate(P) && Domain(P) == T
  T collision_point(const T & x, F f, P p)
  {
    // Precondition: orbit of x under f is finite
    if (!p(x)) return x;

    T slow = x;
    T fast = f(x);

    while (slow != fast)
    {
      if (!p(fast)) return fast;
      fast = f(fast);
      if (!p(fast)) return fast;
      fast = f(fast);

      slow = f(slow);
    }
    return fast;
  }

  template<typename T, typename F, typename P>
  // requires Transformation(F) && Domain(F) == T &&
  //   Predicate(P) && Domain(P) == T
  bool is_terminating(const T & x, F f, P p)
  {
    // Precondition: orbit of x under f is finite
    return !p(collision_point(x, f, p));
  }

  template<typename T, typename F>
  // requires Transformation(F) && Domain(F) == T
  T collision_point_nonterminating_orbit(const T & x, F f)
  {
    // Precondition: orbit of x under f is circular or rho-shaped

    T slow = x;
    T fast = f(x);

    while (slow != fast)
    {
      fast = f(fast);
      fast = f(fast);

      slow = f(slow);
    }
    return fast;
  }

  template<typename T, typename F>
  // requires Transformation(F) && Domain(F) == T
  bool is_circular_nonterminating_orbit(const T & x, F f)
  {
    // Precondition: orbit of x under f is circular or rho-shaped
    return x == f(collision_point_nonterminating_orbit(x, f));
  }

  template<typename T, typename F, typename P>
  // requires Transformation(F) && Domain(F) == T &&
  //   Predicate(P) && Domain(P) == T
  bool is_circular(const T & x, F f, P p)
  {
    // Precondition: orbit of x under f is finite
    T y = collision_point(x, f, p);
    return p(y) && (x == f(y));
  }

  template<typename T, typename F>
  // requires Transformation(F) && Domain(F) == T
  T convergent_point(T x0, T x1, F f)
  {
    while (x0 != x1)
    {
      x0 = f(x0);
      x1 = f(x1);
    }
    return x0;
  }

  template<typename T, typename F>
  // requires Transformation(F) && Domain(F) == T
  T connection_point_nonterminating_orbit(const T & x, F f)
  {
    // Precondition: orbit of x under f is circular or rho-shaped
    return convergent_point(
      x, f(collision_point_nonterminating_orbit(x, f)), f);
  }

  template<typename T, typename F, typename P>
  // requires Transformation(F) && Domain(F) == T &&
  //   Predicate(P) && Domain(P) == T
  T connection_point(const T & x, F f, P p)
  {
    // Precondition: orbit of x under f is finite
    T y = collision_point(x, f, p);
    if (!p(y)) return y;
    return convergent_point(x, f(y) , f);;
  }

} // namespace eop_02
