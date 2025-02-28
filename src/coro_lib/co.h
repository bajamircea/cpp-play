#pragma once

#include "scoped_coroutine_handle.h"
#include "promise_base.h"

#include <cassert>
#include <coroutine>

namespace coro
{
  template<typename T>
  class [[nodiscard]] co
  {
  public:
    using co_return_type = T;

    class promise_type : public promise_base<T>
    {
      friend co;

      std::coroutine_handle<> parent_coro_;

    public:
      promise_type() noexcept = default;

      promise_type(const promise_type&) = delete;
      promise_type& operator=(const promise_type&) = delete;
  
      co get_return_object() noexcept
      {
        return co{std::coroutine_handle<promise_type>::from_promise(*this)};
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
        return {};
      }
    };

  private:
    scoped_coroutine_handle<promise_type> scoped_child_coro_;

    co(std::coroutine_handle<promise_type> child_coro) noexcept : scoped_child_coro_{ child_coro }
    {
    }

    co(const co&) = delete;
    co& operator=(const co&) = delete;

    struct co_awaiter
    {
      std::coroutine_handle<promise_type> child_coro_;

      [[nodiscard]] constexpr bool await_ready() const noexcept
      {
        return false;
      }

      auto await_suspend(std::coroutine_handle<> parent_coro) noexcept
      {
        assert(!child_coro_.promise().parent_coro_);
        child_coro_.promise().parent_coro_ = parent_coro;
        return child_coro_;
      }

      T await_resume()
      {
        return child_coro_.promise().get_result();
      }
    };

  public:
    friend co_awaiter operator co_await(co x)
    {
      return co_awaiter{ x.scoped_child_coro_.get() };
    }
  };
}
