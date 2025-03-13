#pragma once

#include "deferred_type_traits.h"
#include "st_context.h"

namespace coro::st
{
 template <typename T>
  concept is_deferred_context_co = is_deferred_co<T, coro::st::context&>;

  template <typename T>
  concept deferred_context_co_has_non_member_operator_co_await =
    deferred_co_has_non_member_operator_co_await<T, coro::st::context&>;

  template <typename T>
  using deferred_context_co_return_type = deferred_co_return_type<T, coro::st::context&>;
}