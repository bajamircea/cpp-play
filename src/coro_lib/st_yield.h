#pragma once

#include "st_context.h"

#include <coroutine>

namespace coro::st
{
  class [[nodiscard]] yield_awaitable
  {
  public:
    using co_return_type = void;

  private:
    context& ctx_;

  public:
    yield_awaitable(context& ctx) noexcept :
      ctx_{ ctx }
    {
    }

    yield_awaitable(const yield_awaitable&) = delete;
    yield_awaitable& operator=(const yield_awaitable&) = delete;

  private:
    class [[nodiscard]] awaiter
    {
      context& ctx_;
      ready_node node_;

    public:
      awaiter(context& ctx) noexcept :
        ctx_{ ctx },
        node_{}
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
        ctx_.push_ready_node(node_, handle);
      }

      constexpr co_return_type await_resume() const noexcept
      {
      }
    };

    [[nodiscard]] friend awaiter operator co_await(yield_awaitable x) noexcept
    {
      return { x.ctx_ };
    }
  };

  [[nodiscard]] inline yield_awaitable async_yield(context& ctx) noexcept
  {
    return { ctx };
  }
}
