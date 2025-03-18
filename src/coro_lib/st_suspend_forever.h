#pragma once

#include "st_context.h"
#include "st_stop_op_callback.h"

#include <coroutine>

namespace coro::st
{
  class [[nodiscard]] suspend_forever_awaitable
  {
  public:
    using co_return_type = void;

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
      stop_op_callback<awaiter> stop_cb_;
      ready_node node_;  

    public:
      awaiter(context& ctx) noexcept :
        ctx_{ ctx },
        stop_cb_{},
        node_{}
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

      constexpr co_return_type await_resume() const noexcept
      {
      }

      void cancel() noexcept
      {
        stop_cb_.disable();
        ctx_.push_ready_node(node_, std::coroutine_handle<>());
      }  
    };

    [[nodiscard]] friend awaiter operator co_await(suspend_forever_awaitable x) noexcept
    {
      return { x.ctx_ };
    }
  };

  [[nodiscard]] inline suspend_forever_awaitable async_suspend_forever(context& ctx) noexcept
  {
    return { ctx };
  }
}
