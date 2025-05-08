#pragma once

#include "callback.h"
#include "context.h"
#include "stop_util.h"

#include <coroutine>
#include <optional>

namespace coro_st
{
  class [[nodiscard]] suspend_forever_task
  {
  public:
    suspend_forever_task() noexcept = default;

    suspend_forever_task(const suspend_forever_task&) = delete;
    suspend_forever_task& operator=(const suspend_forever_task&) = delete;

  private:
    class [[nodiscard]] awaiter
    {
      context& ctx_;
      std::optional<stop_callback<callback>> stop_cb_;

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

      void await_suspend(std::coroutine_handle<>) noexcept
      {
        configure_cancellation();
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
        configure_cancellation();
      }

    private:
      void configure_cancellation() noexcept
      {
        stop_cb_.emplace(
          ctx_.get_stop_token(),
          make_member_callback<&awaiter::on_cancel>(this));
      }

      void on_cancel() noexcept
      {
        stop_cb_.reset();
        ctx_.schedule_cancellation_callback();
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

  [[nodiscard]] inline suspend_forever_task async_suspend_forever() noexcept
  {
    return {};
  }
}
