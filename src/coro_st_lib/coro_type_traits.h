#pragma once

#include "context.h"

#include <coroutine>
#include <exception>
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
      { a.get_result_exception() } noexcept -> std::same_as<std::exception_ptr>;
    } && (
      has_void_await_suspend<T> ||
      has_bool_await_suspend<T> ||
      has_symmetric_await_suspend<T>) &&
    !std::is_move_constructible_v<T> &&
    !std::is_move_assignable_v<T>;

  template<is_co_awaiter T>
  using co_awaiter_result_t = decltype(
    std::declval<T>().await_resume());

  template<typename T>
  concept is_co_work = requires(T x, context ctx)
    {
      // removed noexcept, in some cases allocation is required
      { x.get_awaiter(ctx) } -> is_co_awaiter;
    } &&
    std::is_nothrow_move_constructible_v<T> &&
    std::is_nothrow_move_assignable_v<T>;

  template<is_co_work T>
  using co_work_awaiter_t = decltype(
    std::declval<T>().get_awaiter(std::declval<context&>()));

  template<is_co_work T>
  using co_work_result_t = co_awaiter_result_t<co_work_awaiter_t<T>>;

  template<typename T>
  concept is_co_task = requires(T x)
    {
      { x.get_work() } noexcept -> is_co_work;
    } &&
    !std::is_move_constructible_v<T> &&
    !std::is_move_assignable_v<T>;

  template<is_co_task T>
  using co_task_work_t = decltype(
    std::declval<T>().get_work());

  template<is_co_task T>
  using co_task_awaiter_t = co_work_awaiter_t<co_task_work_t<T>>;

  template<is_co_task T>
  using co_task_result_t = co_awaiter_result_t<co_task_awaiter_t<T>>;
}