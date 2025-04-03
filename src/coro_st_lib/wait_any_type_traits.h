#pragma once

#include <tuple>
#include <type_traits>

namespace coro_st::wait_any_type_traits
{
  // primary template, used for no types
  template<typename...>
  struct value_type
  {
  };

  template<typename T>
  struct value_type<T>
  {
    using type = T;
  };

  template<typename T0, typename T1>
  struct value_type<T0, T1>
  {
    using type = std::common_type_t<T0, T1>;
  };

  template<typename T1>
  struct value_type<void, T1>
  {
    using type = T1;
  };

  template<typename T0>
  struct value_type<T0, void>
  {
    using type = T0;
  };

  template<>
  struct value_type<void, void>
  {
    using type = void;
  };

  template<typename T0, typename T1, typename... R>
  struct value_type<T0, T1, R...> :
    public value_type<typename value_type<T0, T1>::type, R...>
  {
  };

  template<typename...T>
  using value_type_t = value_type<T...>::type;
}