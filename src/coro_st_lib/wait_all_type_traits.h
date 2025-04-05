#pragma once

#include "void_result.h"

namespace coro_st::wait_all_type_traits
{
  template<typename T>
  struct value_type
  {
    using type = T;
  };

  template<>
  struct value_type<void>
  {
    using type = void_result;
  };

  template<typename...T>
  using value_type_t = value_type<T...>::type;
}