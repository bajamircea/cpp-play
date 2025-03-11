#pragma once

#include "st_context.h"

#include <coroutine>

namespace coro::st
{
  // TODO: create a suspend_forever
  // TODO: add cancellation to suspend_forever
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

  [[nodiscard]] inline yield_awaiter async_yield(context& ctx) noexcept
  {
    return { ctx };
  }
}
