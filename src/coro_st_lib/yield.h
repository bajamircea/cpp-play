#pragma once

#include "context.h"

#include <coroutine>

namespace coro_st
{
  class [[nodiscard]] yield_task
  {
  public:
    yield_task() noexcept = default;

    yield_task(const yield_task&) = delete;
    yield_task& operator=(const yield_task&) = delete;

  private:
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

      [[nodiscard]] constexpr bool await_ready() const noexcept
      {
        return false;
      }

      void await_suspend(std::coroutine_handle<> handle) noexcept
      {
        if (ctx_.get_stop_token().stop_requested())
        {
          ctx_.schedule_cancellation_callback();
          return;
        }

        ready_node& n = ctx_.get_chain_node();
        n.cb = make_resume_coroutine_callback(handle);
        ctx_.push_ready_node(n);
      }

      constexpr void await_resume() const noexcept
      {
      }

      void start_as_chain_root() noexcept
      {
        ctx_.schedule_continuation_callback();
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
    [[nodiscard]] work get_work() noexcept
    {
      return {};
    }
  };

  [[nodiscard]] inline yield_task async_yield() noexcept
  {
    return {};
  }
}
