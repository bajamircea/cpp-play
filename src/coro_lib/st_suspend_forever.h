#pragma once

#include "st_context.h"
#include "st_stop_op_callback.h"

#include <coroutine>

namespace coro::st
{
  // TODO: add tests to suspend_forever (requires some async_wait_any or nursery)
  class [[nodiscard]] suspend_forever_awaiter
  {
  public:
    using co_return_type = void;

  private:
    context& ctx_;
    stop_op_callback<suspend_forever_awaiter> stop_cb_;
    ready_node node_;

  public:
    suspend_forever_awaiter(context& ctx) noexcept :
      ctx_{ ctx }
    {
    }

    suspend_forever_awaiter(const suspend_forever_awaiter&) = delete;
    suspend_forever_awaiter& operator=(const suspend_forever_awaiter&) = delete;

    void await_suspend_impl(std::coroutine_handle<>) noexcept
    {
      stop_cb_.enable(ctx_.get_stop_token(), &suspend_forever_awaiter::cancel, this);
    }

    void cancel() noexcept
    {
      stop_cb_.disable();
      ctx_.push_ready_node(node_, std::coroutine_handle<>());
    }


  private:
    class [[nodiscard]] awaiter
    {
      suspend_forever_awaiter& impl_;

    public:
      awaiter(suspend_forever_awaiter& impl) noexcept : impl_{ impl }
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

    [[nodiscard]] friend awaiter operator co_await(suspend_forever_awaiter x) noexcept
    {
      return { x };
    }
  };

  [[nodiscard]] inline suspend_forever_awaiter async_suspend_forever(context& ctx) noexcept
  {
    return { ctx };
  }
}
