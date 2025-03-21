#pragma once

#include "coro_type_traits.h"
#include "st_context.h"

namespace coro::st
{
  template <typename Fn>
  concept is_context_callable_co = requires(Fn fn, context& ctx)
  {
    { coro::get_awaiter(fn(ctx)) } -> coro::is_awaiter;
  };

  template <typename Fn>
  using context_callable_awaiter_t =
    awaitable_traits<
      std::invoke_result_t<Fn, context&>>::awaiter_t;

  template <typename Fn>
  using context_callable_await_result_t =
    awaitable_traits<
      std::invoke_result_t<Fn, context&>>::await_result_t;
}