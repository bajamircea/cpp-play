#pragma once

#include "st.h"

#include <chrono>
#include <thread>

namespace coro::st
{
  class [[nodiscard]] yield_awaiter
  {
    context& ctx_;
    ready_node node_;
    
  public:
    yield_awaiter(context& ctx) noexcept :
      ctx_{ ctx }
    {
    }

    yield_awaiter(const yield_awaiter&) = delete;
    yield_awaiter& operator=(const yield_awaiter&) = delete;

  private:
    void await_suspend_impl(std::coroutine_handle<> handle) noexcept
    {
      ctx_.push_ready_node(node_, handle);
    }

    class [[nodiscard]] awaiter
    {
      yield_awaiter& impl_;

    public:
      awaiter(yield_awaiter& impl) noexcept : impl_{ impl }
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

    [[nodiscard]] friend awaiter operator co_await(yield_awaiter x) noexcept
    {
      return { x };
    }
  };

  [[nodiscard]] yield_awaiter async_yield(context& ctx) noexcept
  {
    return yield_awaiter{ ctx };
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
      ctx_.insert_timer_node(node_, handle);
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
