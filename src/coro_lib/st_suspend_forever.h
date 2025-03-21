#pragma once

#include "st_context.h"
#include "st_stop_op_callback.h"

#include <coroutine>

namespace coro::st
{
  class [[nodiscard]] suspend_forever_awaitable
  {
  private:
    context& ctx_;

  public:
    suspend_forever_awaitable(context& ctx) noexcept :
      ctx_{ ctx }
    {
    }

    suspend_forever_awaitable(const suspend_forever_awaitable&) = delete;
    suspend_forever_awaitable& operator=(const suspend_forever_awaitable&) = delete;

  private:
    class [[nodiscard]] awaiter
    {
      context& ctx_;
      ready_node node_;
      stop_op_callback<awaiter> stop_cb_;

    public:
      awaiter(context& ctx) noexcept :
        ctx_{ ctx },
        node_{},
        stop_cb_{}
      {
      }

      awaiter(const awaiter&) = delete;
      awaiter& operator=(const awaiter&) = delete;

      [[nodiscard]] constexpr bool await_ready() const noexcept
      {
        return false;
      }

      void await_suspend(std::coroutine_handle<>) noexcept
      {
        stop_cb_.enable(ctx_.get_stop_token(), &awaiter::cancel, this);
      }

      constexpr void await_resume() const noexcept
      {
      }

      void cancel() noexcept
      {
        stop_cb_.disable();
        ctx_.push_ready_node(node_, std::coroutine_handle<>());
      }
    };

  private:
    [[nodiscard]] friend awaiter operator co_await(suspend_forever_awaitable x) noexcept
    {
      return std::move(x).hazmat_get_awaiter();
    }

    public:
    [[nodiscard]] awaiter hazmat_get_awaiter() && noexcept
    {
      return { ctx_ };
    }
  };

  [[nodiscard]] inline suspend_forever_awaitable async_suspend_forever(context& ctx) noexcept
  {
    return { ctx };
  }
}
