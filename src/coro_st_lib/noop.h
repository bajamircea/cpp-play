#pragma once

#include "context.h"

#include <coroutine>

namespace coro_st
{
  class [[nodiscard]] noop_task
  {
    class [[nodiscard]] awaiter
    {
      context& ctx_;

    public:
      awaiter(context& ctx) noexcept :
        ctx_{ ctx }
      {
      }

      awaiter(const awaiter&) = delete;
      awaiter& operator=(const awaiter&) = delete;

      [[nodiscard]] bool await_ready() const noexcept
      {
        return !ctx_.get_stop_token().stop_requested();
      }

      void await_suspend(std::coroutine_handle<>) noexcept
      {
        ctx_.schedule_cancellation();
      }

      constexpr void await_resume() const noexcept
      {
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        return {};
      }

      void start_as_chain_root() noexcept
      {
        if (ctx_.get_stop_token().stop_requested())
        {
          ctx_.invoke_cancellation();
          return;
        }

        ctx_.invoke_continuation();
      }
    };

    struct [[nodiscard]] work
    {
      work() noexcept = default;

      work(const work&) = delete;
      work& operator=(const work&) = delete;
      work(work&&) noexcept = default;
      work& operator=(work&&) noexcept = default;

      [[nodiscard]] awaiter get_awaiter(context& ctx) noexcept
      {
        return { ctx };
      }
    };

  public:
    noop_task() noexcept = default;

    noop_task(const noop_task&) = delete;
    noop_task& operator=(const noop_task&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return {};
    }
  };

  [[nodiscard]] inline noop_task async_noop() noexcept
  {
    return {};
  }
}
