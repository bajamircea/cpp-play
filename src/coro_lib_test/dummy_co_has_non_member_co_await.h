#pragma once

#include "../coro_lib/unique_coroutine_handle.h"
#include "../coro_lib/promise_base.h"

#include <cassert>
#include <coroutine>
#include <utility>

namespace coro_test
{
  template<typename T>
  class [[nodiscard]] dummy_co_has_non_member_co_await
  {
  public:
    using co_return_type = T;

    class promise_type : public coro::promise_base<T>
    {
      friend dummy_co_has_non_member_co_await;

      std::coroutine_handle<> parent_coro_;

    public:
      promise_type() noexcept = default;

      promise_type(const promise_type&) = delete;
      promise_type& operator=(const promise_type&) = delete;

      dummy_co_has_non_member_co_await get_return_object() noexcept
      {
        return { std::coroutine_handle<promise_type>::from_promise(*this) };
      }

      std::suspend_always initial_suspend() noexcept
      {
        return {};
      }

      struct final_awaiter
      {
        [[nodiscard]] constexpr bool await_ready() const noexcept
        {
          return false;
        }

        auto await_suspend(std::coroutine_handle<promise_type> child_coro) noexcept
        {
          return child_coro.promise().parent_coro_;
        }

        [[noreturn]] constexpr void await_resume() const noexcept
        {
          std::unreachable();
          //std::terminate();
        }
      };

      final_awaiter final_suspend() noexcept
      {
        assert(parent_coro_);
        return {};
      }
    };

  private:
    coro::unique_coroutine_handle<promise_type> unique_child_coro_;

    dummy_co_has_non_member_co_await(std::coroutine_handle<promise_type> child_coro) noexcept :
      unique_child_coro_{ child_coro }
    {
    }

  public:
    dummy_co_has_non_member_co_await(const dummy_co_has_non_member_co_await&) = delete;
    dummy_co_has_non_member_co_await& operator=(const dummy_co_has_non_member_co_await&) = delete;

  private:
    class [[nodiscard]] awaiter
    {
      coro::unique_coroutine_handle<promise_type> unique_child_coro_;

    public:
      awaiter(coro::unique_coroutine_handle<promise_type>&& unique_child_coro) noexcept :
        unique_child_coro_{ std::move(unique_child_coro) }
      {
      }

      awaiter(const awaiter&) = delete;
      awaiter& operator=(const awaiter&) = delete;

// TODO: maybe uncomment
// #ifdef _DEBUG
//       ~awaiter()
//       {
//         assert(unique_child_coro_.get().promise().parent_coro_);
//       }
// #endif

      [[nodiscard]] constexpr bool await_ready() const noexcept
      {
        return false;
      }

      std::coroutine_handle<promise_type> await_suspend(std::coroutine_handle<> parent_coro) noexcept
      {
        std::coroutine_handle<promise_type> child_coro = unique_child_coro_.get();
        assert(!child_coro.promise().parent_coro_);
        child_coro.promise().parent_coro_ = parent_coro;
        return child_coro;
      }

      T await_resume() const
      {
        return unique_child_coro_.get().promise().get_result();
      }
    };

  public:
    [[nodiscard]] friend awaiter operator co_await(dummy_co_has_non_member_co_await&& x) noexcept
    {
      return { std::move(x.unique_child_coro_) };
    }
  };
}