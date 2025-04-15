#pragma once

#include "callback.h"
#include "context.h"
#include "coro_type_traits.h"
#include "stop_util.h"

#include <chrono>
#include <coroutine>
#include <type_traits>
#include <optional>

namespace coro_st
{
  template<is_co_task CoTask>
  class [[nodiscard]] wait_for_task
  {
    using CoWork = co_task_work_t<CoTask>;
    using CoAwaiter = co_task_awaiter_t<CoTask>;
    using T = co_task_result_t<CoTask>;
    using ResultType = std::conditional_t<
      std::is_same_v<void, T>, bool, T>;

    CoWork co_work_;
    std::chrono::steady_clock::time_point deadline_;

  public:
    wait_for_task(
      CoTask& co_task,
      std::chrono::steady_clock::time_point deadline
    ) noexcept :
      co_work_{ co_task.get_work() },
      deadline_{ deadline }
    {
    }

    wait_for_task(const wait_for_task&) = delete;
    wait_for_task& operator=(const wait_for_task&) = delete;

  private:
    class [[nodiscard]] awaiter
    {
      context& parent_ctx_;
      std::coroutine_handle<> parent_handle_;
      stop_source wait_stop_source_;
      std::optional<stop_callback<callback>> wait_stop_cb_;
      size_t pending_count_{ 0 };
      bool has_result_{ false };

      chain_context task_chain_ctx_;
      context task_ctx_;
      CoAwaiter co_awaiter_;

      timer_node timer_node_;
      std::optional<stop_callback<callback>> timer_stop_cb_;

    public:
      awaiter(
        context& parent_ctx,
        CoWork& co_work,
        std::chrono::steady_clock::time_point deadline
      ) noexcept :
        parent_ctx_{ parent_ctx },
        parent_handle_{},
        wait_stop_source_{},
        wait_stop_cb_{},
        pending_count_{},
        has_result_{ false },
        task_chain_ctx_{
          wait_stop_source_.get_token(),
          make_member_callback<&awaiter::on_task_chain_continue>(this),
          make_member_callback<&awaiter::on_task_chain_cancel>(this),
          },
        task_ctx_{ parent_ctx_, task_chain_ctx_ },
        co_awaiter_{ co_work.get_awaiter(task_ctx_) },
        timer_node_{ deadline },
        timer_stop_cb_{ std::nullopt }
      {
      }

      awaiter(const awaiter&) = delete;
      awaiter& operator=(const awaiter&) = delete;

      [[nodiscard]] constexpr bool await_ready() const noexcept
      {
        return false;
      }

      bool await_suspend(std::coroutine_handle<> handle) noexcept
      {
        parent_handle_ = handle;

        pending_count_ = 3;
        init_parent_cancellation_callback();

        start_chains();

        --pending_count_;
        if (0 != pending_count_)
        {
          return true;
        }

        wait_stop_cb_ = std::nullopt;

        if (parent_ctx_.get_stop_token().stop_requested())
        {
          parent_ctx_.schedule_cancellation_callback();
          return true;
        }

        return false;
      }

      ResultType await_resume() const
      {
        if constexpr (std::is_same_v<void, T>)
        {
          if (!has_result_)
          {
            return false;
          }
          co_awaiter_.await_resume();
          return true;
        }
        else
        {
          if (!has_result_)
          {
            return std::nullopt;
          }
          return co_awaiter_.await_resume();
        }
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        if (!has_result_)
        {
          // was cancelled
          return {};
        }
        return co_awaiter_.get_result_exception();
      }

      void start_as_chain_root() noexcept
      {
        pending_count_ = 3;
        init_parent_cancellation_callback();

        start_chains();

        --pending_count_;
        if (0 != pending_count_)
        {
          return;
        }

        wait_stop_cb_ = std::nullopt;

        if (parent_ctx_.get_stop_token().stop_requested())
        {
          parent_ctx_.schedule_cancellation_callback();
          return;
        }

        callback continuation_cb = parent_ctx_.get_continuation_callback();
        continuation_cb.invoke();
      }

    private:
      void on_shared_continue() noexcept
      {
        wait_stop_cb_ = std::nullopt;

        if (parent_ctx_.get_stop_token().stop_requested())
        {
          parent_ctx_.schedule_cancellation_callback();
          return;
        }

        if (parent_handle_)
        {
          parent_ctx_.schedule_coroutine_resume(parent_handle_);
          return;
        }

        parent_ctx_.schedule_continuation_callback();
      }

      void on_task_chain_continue() noexcept
      {
        if (!parent_ctx_.get_stop_token().stop_requested())
        {
          has_result_ = true;
          wait_stop_source_.request_stop();
        }

        --pending_count_;
        if (0 != pending_count_)
        {
          return;
        }
        on_shared_continue();
      }

      void on_task_chain_cancel() noexcept
      {
        --pending_count_;
        if (0 != pending_count_)
        {
          return;
        }
        on_shared_continue();
      }

      void init_parent_cancellation_callback() noexcept
      {
        wait_stop_cb_.emplace(
          parent_ctx_.get_stop_token(),
          make_member_callback<&awaiter::on_parent_cancel>(this));
      }

      void on_parent_cancel() noexcept
      {
        wait_stop_cb_ = std::nullopt;
        wait_stop_source_.request_stop();
      }

      void start_chains() noexcept
      {
        co_awaiter_.start_as_chain_root();

        if (2 == pending_count_)
        {
          --pending_count_;
          // co_awaiter_ completed early, no need to sleep
          return;
        }

        schedule_timer();
      }

      void schedule_timer() noexcept
      {
        timer_node_.cb = make_member_callback<&awaiter::on_timer>(this);
        parent_ctx_.insert_timer_node(timer_node_);
        timer_stop_cb_.emplace(
          wait_stop_source_.get_token(),
          make_member_callback<&awaiter::on_timer_cancel>(this));
      }

      void on_timer() noexcept
      {
        timer_stop_cb_ = std::nullopt;

        if (!parent_ctx_.get_stop_token().stop_requested())
        {
          wait_stop_source_.request_stop();
        }

        --pending_count_;
        if (0 != pending_count_)
        {
          return;
        }

        wait_stop_cb_ = std::nullopt;

        if (parent_ctx_.get_stop_token().stop_requested())
        {
          parent_ctx_.schedule_cancellation_callback();
          return;
        }

        if (parent_handle_)
        {
          parent_ctx_.schedule_coroutine_resume(parent_handle_);
          return;
        }

        callback continuation_cb = parent_ctx_.get_continuation_callback();
        continuation_cb.invoke();
      }

      void on_timer_cancel() noexcept
      {
        timer_stop_cb_ = std::nullopt;
        parent_ctx_.remove_timer_node(timer_node_);

        --pending_count_;
        if (0 != pending_count_)
        {
          return;
        }
        on_shared_continue();
      }
    };

    struct [[nodiscard]] work
    {
      CoWork co_work_;
      std::chrono::steady_clock::time_point deadline_;

      work(
        CoWork&& co_work_,
        std::chrono::steady_clock::time_point deadline
      ) noexcept:
        co_work_{ std::move(co_work_) },
        deadline_{ deadline }
      {
      }

      work(const work&) = delete;
      work& operator=(const work&) = delete;
      work(work&&) noexcept = default;
      work& operator=(work&&) noexcept = default;

      [[nodiscard]] awaiter get_awaiter(context& ctx) noexcept
      {
        return awaiter(ctx, co_work_, deadline_);
      }
    };

  public:
    [[nodiscard]] work get_work() noexcept
    {
      return { std::move(co_work_), deadline_ };
    }
  };

  template<is_co_task CoTask>
  [[nodiscard]] wait_for_task<CoTask>
    async_wait_for(CoTask co_task, std::chrono::steady_clock::duration sleep_duration)
  {
    return wait_for_task<CoTask>{ co_task, std::chrono::steady_clock::now() + sleep_duration };
  }
}