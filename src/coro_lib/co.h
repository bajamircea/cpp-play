#pragma once

#include "unique_coroutine_handle.h"
#include "promise_base.h"

#include <cassert>
#include <coroutine>
#include <utility>

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
    unique_coroutine_handle<promise_type> unique_child_coro_;

    co(std::coroutine_handle<promise_type> child_coro) noexcept : unique_child_coro_{ child_coro }
    {
    }

  public:
    co(const co&) = delete;
    co& operator=(const co&) = delete;

#ifdef _DEBUG
    ~co()
    {
      assert(unique_child_coro_.get().promise().parent_coro_);
    }
#endif

  private:
    auto await_suspend_impl(std::coroutine_handle<> parent_coro) noexcept
    {
      std::coroutine_handle<promise_type> child_coro = unique_child_coro_.get();
      assert(!child_coro.promise().parent_coro_);
      child_coro.promise().parent_coro_ = parent_coro;
      return child_coro;
    }

    T await_resume_impl()
    {
      return unique_child_coro_.get().promise().get_result();
    }

    class [[nodiscard]] awaiter
    {
      co& impl_;

    public:
      awaiter(co& impl) noexcept : impl_{ impl }
      {
      }

      awaiter(const awaiter&) = delete;
      awaiter& operator=(const awaiter&) = delete;

      [[nodiscard]] constexpr bool await_ready() const noexcept
      {
        return false;
      }

      auto await_suspend(std::coroutine_handle<> parent_coro) noexcept
      {
        return impl_.await_suspend_impl(parent_coro);
      }

      T await_resume()
      {
        return impl_.await_resume_impl();
      }
    };

    friend awaiter;

    [[nodiscard]] friend awaiter operator co_await(co x) noexcept
    {
      return { x };
    }
  };
}