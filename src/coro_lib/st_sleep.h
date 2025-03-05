#pragma once

#include "st.h"

#include <chrono>
#include <thread>

namespace coro::st
{
  struct defer_awaiter
  {
    context& ctx_;
    ready_node node_;

    defer_awaiter(context& ctx) noexcept :
      ctx_{ ctx }
    {
    }

    [[nodiscard]] constexpr bool await_ready() const noexcept
    {
      return false;
    }

    void await_suspend(std::coroutine_handle<> handle) noexcept
    {
      ctx_.hazmat_push_ready_node(node_, handle);
    }

    constexpr void await_resume() const noexcept
    {
    }
  };

  defer_awaiter async_defer(context& ctx) noexcept
  {
    return defer_awaiter(ctx);
  }

  class sleep_awaiter
  {
    context& ctx_;
    timer_node node_;

  public:
    sleep_awaiter(context& ctx, std::chrono::steady_clock::time_point deadline) noexcept :
      ctx_{ ctx }
    {
      node_.deadline = deadline;
    }

    sleep_awaiter(const sleep_awaiter&) = delete;
    sleep_awaiter& operator=(const sleep_awaiter&) = delete;

  private:
    void await_suspend_impl(std::coroutine_handle<> handle) noexcept
    {
      ctx_.hazmat_insert_timer_node(node_, handle);
    }

  private:
    struct awaiter
    {
      sleep_awaiter& impl;

      [[nodiscard]] constexpr bool await_ready() const noexcept
      {
        return false;
      }

      void await_suspend(std::coroutine_handle<> handle) noexcept
      {
        return impl.await_suspend_impl(handle);
      }

      constexpr void await_resume() const noexcept
      {
      }
    };

    friend awaiter;

    friend awaiter operator co_await(sleep_awaiter x)
    {
      return awaiter{ x };
    }
  };

  [[nodiscard]] sleep_awaiter async_sleep(context& ctx, std::chrono::steady_clock::duration sleep_duration) noexcept
  {
    return sleep_awaiter(ctx, std::chrono::steady_clock::now() + sleep_duration);
  }
}
