#pragma once

#include "context.h"

#include <coroutine>
#include <type_traits>
#include <utility>

namespace coro_st
{
  template<typename Awaiter>
  concept has_void_await_suspend = requires(Awaiter a, std::coroutine_handle<> h)
  {
    { a.await_suspend(h) } noexcept -> std::same_as<void>;
  };

  template<typename Awaiter>
  concept has_bool_await_suspend = requires(Awaiter a, std::coroutine_handle<> h)
  {
    { a.await_suspend(h) } noexcept -> std::convertible_to<bool>;
  };

  template<typename Awaiter>
  concept has_symmetric_await_suspend = requires(Awaiter a, std::coroutine_handle<> h)
  {
    { a.await_suspend(h) } noexcept -> std::convertible_to<std::coroutine_handle<>>;
  };

  template<typename T>
  concept is_co_awaiter = requires(T a)
  {
    { a.await_ready() } noexcept -> std::convertible_to<bool>;
    a.await_resume();
    { a.start_as_chain_root() } noexcept;
  } && (
    has_void_await_suspend<T> ||
    has_bool_await_suspend<T> ||
    has_symmetric_await_suspend<T>);

  template<typename T>
  concept is_co_awaitable = requires(T x, context ctx)
  {
    { x.get_awaiter_for_context(ctx) } noexcept -> is_co_awaiter;
  };

  template<is_co_awaitable T>
  using co_awaitable_awaiter_t = decltype(
    std::declval<T>().get_awaiter_for_context(std::declval<context&>()));

  template<is_co_awaitable T>
  using co_awaitable_result_t = decltype(
    std::declval<co_awaitable_awaiter_t<T>>().await_resume());
}