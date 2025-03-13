#pragma once

#include <coroutine>
#include <type_traits>

namespace coro::st
{
  namespace impl
  {
    struct awaiter_no_promise_transform
    {
      struct promise_type
      {
        constexpr std::suspend_always initial_suspend() const
        {
          return {};
        }
        constexpr std::suspend_always final_suspend() const noexcept
        {
          return {};
        }
        void unhandled_exception()
        {
        }
        awaiter_no_promise_transform get_return_object()
        {
          return {};
        }
      };
    };
  }

  template <typename T, typename... Args>
  concept deferred_co =
    std::is_class_v<T> &&
    requires() {
      [](T&& t, Args&&... args) -> coro::st::impl::awaiter_no_promise_transform
      {
        co_await t(args...);
      };
    };

  template <typename T, typename... Args>
  concept deferred_co_has_non_member_operator_co_await =
    deferred_co<T, Args...> &&
    requires(T&& t, Args&&... args) {
      operator co_await(t(args...));
    };

  template <typename T, typename... Args>
  using deferred_co_return_type = std::invoke_result_t<T, Args...>::co_return_type; 
}