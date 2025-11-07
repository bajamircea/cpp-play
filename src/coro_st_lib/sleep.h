#pragma once

#include "callback.h"
#include "context.h"
#include "stop_util.h"

#include <chrono>
#include <coroutine>
#include <optional>

namespace coro_st
{
  class [[nodiscard]] sleep_task
  {
    class [[nodiscard]] awaiter
    {
      context& ctx_;
      timer_node timer_node_;
      std::coroutine_handle<> parent_handle_;
      std::optional<stop_callback<callback>> parent_stop_cb_;

    public:
      awaiter(context& ctx, std::chrono::steady_clock::time_point deadline) noexcept :
        ctx_{ ctx },
        timer_node_{ deadline },
        parent_handle_{},
        parent_stop_cb_{ std::nullopt }
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
        parent_handle_ = handle;
        schedule_timer();
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
        schedule_timer();
      }

    private:
      void schedule_timer() noexcept
      {
        timer_node_.cb = make_member_callback<&awaiter::on_timer>(this);
        ctx_.insert_timer_node(timer_node_);
        parent_stop_cb_.emplace(
          ctx_.get_stop_token(),
          make_member_callback<&awaiter::on_cancel>(this));
      }

      void on_timer() noexcept
      {
        parent_stop_cb_.reset();

        if (parent_handle_)
        {
          parent_handle_.resume();
          return;
        }

        // it's fine to invoke since on_timer is called from the runner loop
        ctx_.invoke_continuation();
      }

      void on_cancel() noexcept
      {
        parent_stop_cb_.reset();
        ctx_.remove_timer_node(timer_node_);
        ctx_.schedule_cancellation();
      }
    };

    class [[nodiscard]] work
    {
      std::chrono::steady_clock::time_point deadline_;

    public:
      work(std::chrono::steady_clock::time_point deadline) noexcept :
        deadline_{ deadline }
      {
      }

      work(const work&) = delete;
      work& operator=(const work&) = delete;
      work(work&&) noexcept = default;
      work& operator=(work&&) noexcept = default;

      [[nodiscard]] awaiter get_awaiter(context& ctx) noexcept
      {
        return { ctx, deadline_ };
      }
    };

  private:
    work work_;

  public:
    sleep_task(std::chrono::steady_clock::time_point deadline) noexcept :
      work_{ deadline }
    {
    }

    sleep_task(const sleep_task&) = delete;
    sleep_task& operator=(const sleep_task&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return std::move(work_);
    }
  };

  [[nodiscard]] inline sleep_task async_sleep_for(std::chrono::steady_clock::duration sleep_duration) noexcept
  {
    return { std::chrono::steady_clock::now() + sleep_duration };
  }
}
