#pragma once

#include "st_context.h"
#include "st_stop_op_callback.h"

#include <chrono>
#include <coroutine>

namespace coro::st
{
  // TODO: make the resumption of a timer more realistic, even if shortcuts
  // can be taken for the single threaded model
  class [[nodiscard]] sleep_awaitable
  {
  public:
    using co_return_type = void;

  private:
    context& ctx_;
    std::chrono::steady_clock::time_point deadline_;

  public:
    sleep_awaitable(context& ctx, std::chrono::steady_clock::time_point deadline) noexcept :
      ctx_{ ctx },
      deadline_{ deadline }
    {
    }

    sleep_awaitable(const sleep_awaitable&) = delete;
    sleep_awaitable& operator=(const sleep_awaitable&) = delete;

  private:
    class [[nodiscard]] awaiter
    {
      context& ctx_;
      timer_node timer_node_;
      ready_node ready_node_;
      stop_op_callback<awaiter> stop_cb_;

    public:
      awaiter(context& ctx, std::chrono::steady_clock::time_point deadline) noexcept :
        ctx_{ ctx },
        timer_node_{ deadline },
        ready_node_{},
        stop_cb_{}
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
        ctx_.insert_timer_node(timer_node_, handle);
        stop_cb_.enable(ctx_.get_stop_token(), &awaiter::cancel, this);
      }

      constexpr co_return_type await_resume() const noexcept
      {
      }

      void cancel() noexcept
      {
        stop_cb_.disable();
        ctx_.remove_timer_node(timer_node_);
        ctx_.push_ready_node(ready_node_, std::coroutine_handle<>());
      }
    };

    [[nodiscard]] friend awaiter operator co_await(sleep_awaitable x) noexcept
    {
      return { x.ctx_, x.deadline_ };
    }
  };

  [[nodiscard]] inline sleep_awaitable async_sleep_for(context& ctx, std::chrono::steady_clock::duration sleep_duration) noexcept
  {
    return { ctx, std::chrono::steady_clock::now() + sleep_duration };
  }
}
