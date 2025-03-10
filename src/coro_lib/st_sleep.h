#pragma once

#include "st.h"

#include <chrono>
#include <thread>

// TODO: rename defer to yield

namespace coro::st
{
  class [[nodiscard]] defer_awaiter
  {
    context& ctx_;
    ready_node node_;
    
  public:
    defer_awaiter(context& ctx) noexcept :
      ctx_{ ctx }
    {
    }

    defer_awaiter(const defer_awaiter&) = delete;
    defer_awaiter& operator=(const defer_awaiter&) = delete;

  private:
    void await_suspend_impl(std::coroutine_handle<> handle) noexcept
    {
      ctx_.hazmat_push_ready_node(node_, handle);
    }

    class [[nodiscard]] awaiter
    {
      defer_awaiter& impl_;

    public:
      awaiter(defer_awaiter& impl) noexcept : impl_{ impl }
      {
      }

      awaiter(const awaiter&) = delete;
      awaiter& operator=(const awaiter&) = delete;

      [[nodiscard]] constexpr bool await_ready() const noexcept
      {
        return false;
      }

      void await_suspend(std::coroutine_handle<> handle) noexcept
      {
        return impl_.await_suspend_impl(handle);
      }

      constexpr void await_resume() const noexcept
      {
      }
    };

    friend awaiter;

    [[nodiscard]] friend awaiter operator co_await(defer_awaiter x) noexcept
    {
      return { x };
    }
  };

  [[nodiscard]] defer_awaiter async_defer(context& ctx) noexcept
  {
    return defer_awaiter{ ctx };
  }

  class [[nodiscard]] sleep_awaiter
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

    class [[nodiscard]] awaiter
    {
      sleep_awaiter& impl_;

    public:
      awaiter(sleep_awaiter& impl) noexcept : impl_{ impl }
      {
      }

      awaiter(const awaiter&) = delete;
      awaiter& operator=(const awaiter&) = delete;

      [[nodiscard]] constexpr bool await_ready() const noexcept
      {
        return false;
      }

      void await_suspend(std::coroutine_handle<> handle) noexcept
      {
        return impl_.await_suspend_impl(handle);
      }

      constexpr void await_resume() const noexcept
      {
      }
    };

    friend awaiter;

    [[nodiscard]] friend awaiter operator co_await(sleep_awaiter x) noexcept
    {
      return { x };
    }
  };

  [[nodiscard]] sleep_awaiter async_sleep(context& ctx, std::chrono::steady_clock::duration sleep_duration) noexcept
  {
    return { ctx, std::chrono::steady_clock::now() + sleep_duration };
  }
}
