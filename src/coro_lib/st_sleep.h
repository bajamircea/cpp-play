#pragma once

#include "st_context.h"
#include "st_stop_op_callback.h"

#include <chrono>
#include <coroutine>

namespace coro::st
{
  class [[nodiscard]] sleep_awaiter
  {
  public:
    using co_return_type = void;

  private:
    context& ctx_;
    timer_node timer_node_;
    stop_op_callback<sleep_awaiter> stop_cb_;
    ready_node ready_node_;

  public:
    sleep_awaiter(context& ctx, std::chrono::steady_clock::time_point deadline) noexcept :
      ctx_{ ctx }
    {
      timer_node_.deadline = deadline;
    }

    sleep_awaiter(const sleep_awaiter&) = delete;
    sleep_awaiter& operator=(const sleep_awaiter&) = delete;

  private:
    void await_suspend_impl(std::coroutine_handle<> handle) noexcept
    {
      ctx_.insert_timer_node(timer_node_, handle);
      stop_cb_.enable(ctx_.get_stop_token(), &sleep_awaiter::cancel, this);
    }

    void cancel() noexcept
    {
      stop_cb_.disable();
      ctx_.remove_timer_node(timer_node_);
      ctx_.push_ready_node(ready_node_, std::coroutine_handle<>());
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

      constexpr co_return_type await_resume() const noexcept
      {
      }
    };

    friend awaiter;

    [[nodiscard]] friend awaiter operator co_await(sleep_awaiter x) noexcept
    {
      return { x };
    }
  };

  [[nodiscard]] inline sleep_awaiter async_sleep(context& ctx, std::chrono::steady_clock::duration sleep_duration) noexcept
  {
    return { ctx, std::chrono::steady_clock::now() + sleep_duration };
  }
}
