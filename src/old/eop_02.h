#pragma once

#include <cstddef> // for size_t
#include <utility> // for std::forward

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

  // simplification: on a 64 bit machine, as I have, size_t is enough
  using distance_type = std::size_t;

  #define DistanceType(F) distance_type

  template<typename T, typename F>
  // requires Transformation(F) && Domain(F) == T
  DistanceType(F) distance(T x, T y, F f)
  {
    // Precondition:
    // - exists i such that f^i(x) == y
    DistanceType(F) result{ 0 };
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

  template<typename T0, typename T1, typename T2>
  struct triple
  {
    template<typename S0, typename S1, typename S2>
    triple(S0 && s0, S1 && s1, S2 && s2) :
      m0{ std::forward<T0>(s0) },
      m1{ std::forward<T1>(s1) },
      m2{ std::forward<T2>(s2) }
    {
    }

    T0 m0;
    T1 m1;
    T2 m2;
  };

  template<typename T0, typename T1, typename T2>
  inline bool operator==(const triple<T0, T1, T2> & lhs, const triple<T0, T1, T2> & rhs)
  {
    return (lhs.m0 == rhs.m0) && (lhs.m1 == rhs.m1) && (lhs.m2 == rhs.m2);
  }

  template<typename T, typename F>
  // requires Tranformation(F) && Domain(F) == T
  triple<DistanceType(F), DistanceType(F), T>
  orbit_structure_nonterminating_orbit(const T & x, F f)
  {
    T y = connection_point_nonterminating_orbit(x, f);
    return triple<DistanceType(F), DistanceType(F), T>(
      distance(x, y, f),
      distance(f(y), y, f),
      y
      );
  }

  template<typename T, typename F, typename P>
  // requires Tranformation(F) && Domain(F) == T
  //   Predicate(P) && Domain(P) == T
  triple<DistanceType(F), DistanceType(F), T>
  orbit_structure(const T & x, F f, P p)
  {
    T y = connection_point(x, f, p);
    DistanceType(F) m0 = distance(x, y, f);
    DistanceType(F) m1 = p(y) ? 
      distance(f(y), y, f) : 0;
    return triple<DistanceType(F), DistanceType(F), T>(m0, m1, y);
  }
} // namespace eop_02
