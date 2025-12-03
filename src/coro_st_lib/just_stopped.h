#pragma once

#include "context.h"

#include <coroutine>

namespace coro_st
{
  class [[nodiscard]] just_stopped_task
  {
    class [[nodiscard]] awaiter
    {
      context& ctx_;

    public:
      explicit awaiter(context& ctx) noexcept :
        ctx_{ ctx }
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
        ctx_.schedule_stopped();
      }

      constexpr void await_resume() const noexcept
      {
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        return {};
      }

      void start() noexcept
      {
        ctx_.invoke_stopped();
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
        return awaiter{ ctx };
      }
    };

  public:
    just_stopped_task() noexcept = default;

    just_stopped_task(const just_stopped_task&) = delete;
    just_stopped_task& operator=(const just_stopped_task&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return {};
    }
  };

  [[nodiscard]] inline just_stopped_task async_just_stopped() noexcept
  {
    return {};
  }
}
