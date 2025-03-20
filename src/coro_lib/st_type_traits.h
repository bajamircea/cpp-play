#pragma once

#include "deferred_type_traits.h"
#include "coro_type_traits.h"
#include "st_context.h"

namespace coro::st
{
  template <typename Fn>
  concept is_context_callable_co = requires(Fn fn, context& ctx)
  {
    { coro::get_awaiter(fn(ctx)) } -> coro::is_awaiter;
  };

  //TODO high delete from here downwards
  template <typename T>
  concept is_deferred_context_co = is_deferred_co<T, coro::st::context&>;

  template <typename T>
  using deferred_context_co_return_type = deferred_co_return_type<T, coro::st::context&>;
}