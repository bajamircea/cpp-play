#pragma once

#include <coroutine>
#include <type_traits>
#include <utility>

namespace coro
{
  template <typename Awaitable>
  concept has_member_operator_co_await =
    requires (Awaitable awaitable) {
      std::move(awaitable).operator co_await();
  };

  template <typename Awaitable>
  concept has_non_member_operator_co_await =
    requires (Awaitable awaitable) {
      operator co_await(std::move(awaitable));
  };

  template <typename Awaitable>
  concept has_get_awaiter =
    requires (Awaitable awaitable) {
      std::move(awaitable).hazmat_get_awaiter();
  };

  template<typename Awaitable>
  decltype(auto) get_awaiter(Awaitable&& awaitable)
  {
    if constexpr (has_get_awaiter<Awaitable>)
    {
      return std::move(awaitable).hazmat_get_awaiter();
    }
    else if constexpr (has_member_operator_co_await<Awaitable>)
    {
      return std::move(awaitable).operator co_await();
    }
    else if constexpr (has_non_member_operator_co_await<Awaitable>)
    {
      return operator co_await(std::move(awaitable));
    }
    else
    {
      return std::move(awaitable);
    }
  }

  template<typename Awaiter>
  concept has_void_await_suspend = requires(Awaiter a, std::coroutine_handle<> h)
  {
    { a.await_suspend(h) } -> std::same_as<void>;
  };

  template<typename Awaiter>
  concept has_bool_await_suspend = requires(Awaiter a, std::coroutine_handle<> h)
  {
    { a.await_suspend(h) } -> std::convertible_to<bool>;
  };

  template<typename Awaiter>
  concept has_symmetric_await_suspend = requires(Awaiter a, std::coroutine_handle<> h)
  {
    { a.await_suspend(h) } -> std::convertible_to<std::coroutine_handle<>>;
  };

  template<typename Awaiter>
  concept is_awaiter = requires(Awaiter a)
  {
    { a.await_ready() } -> std::convertible_to<bool>;
    a.await_resume();
  } && (
    has_void_await_suspend<Awaiter> ||
    has_bool_await_suspend<Awaiter> ||
    has_symmetric_await_suspend<Awaiter>);

  template<typename T, typename=void>
  struct awaitable_traits
  {
  };

  template<typename T>
  struct awaitable_traits<T, std::void_t<decltype(get_awaiter(std::declval<T>()))>>
  {
    using awaiter_t = decltype(get_awaiter(std::declval<T>()));
    using await_result_t = decltype(std::declval<awaiter_t>().await_resume());
  };
}