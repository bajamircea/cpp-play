#pragma once

#include "callback.h"
#include "context.h"
#include "coro_type_traits.h"
#include "stop_util.h"
#include "value_type_traits.h"

#include <cassert>
#include <chrono>
#include <coroutine>
#include <optional>

namespace coro_st
{
  template<is_co_task CoTask>
  class [[nodiscard]] wait_for_task
  {
    using CoWork = co_task_work_t<CoTask>;
    using CoAwaiter = co_task_awaiter_t<CoTask>;
    using T = co_task_result_t<CoTask>;
    using ResultType = std::optional<value_type_traits::value_type_t<T>>;

    class [[nodiscard]] awaiter
    {
      enum class result_state
      {
        none,
        has_result,
        has_stopped,
        has_timeout,
      };

      context& parent_ctx_;
      std::coroutine_handle<> parent_handle_;
      std::optional<stop_callback<callback>> parent_stop_cb_;
      stop_source children_stop_source_;
      size_t pending_count_{ 0 };
      result_state result_state_{ result_state::none };

      context task_ctx_;
      CoAwaiter co_awaiter_;

      timer_node timer_node_;
      std::optional<stop_callback<callback>> timer_stop_cb_;

    public:
      awaiter(
        context& parent_ctx,
        CoWork& co_work,
        std::chrono::steady_clock::time_point deadline
      ) :
        parent_ctx_{ parent_ctx },
        parent_handle_{},
        parent_stop_cb_{},
        children_stop_source_{},
        pending_count_{ 0 },
        result_state_{ result_state::none },
        task_ctx_{
          parent_ctx_,
          children_stop_source_.get_token(),
          make_member_completion<
            &awaiter::on_task_result_ready,
            &awaiter::on_task_stopped
            >(this)
        },
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

        pending_count_ = 1;
        init_parent_cancellation_callback();

        start_chains();

        --pending_count_;
        if (0 != pending_count_)
        {
          return true;
        }

        parent_stop_cb_.reset();

        if (result_state::has_stopped == result_state_)
        {
          parent_ctx_.invoke_stopped();
          return true;
        }

        return false;
      }

      ResultType await_resume()
      {
        switch (result_state_)
        {
          case result_state::none:
          case result_state::has_stopped:
            std::terminate();
          case result_state::has_result:
            break;
          case result_state::has_timeout:
            return std::nullopt;
        }

        if constexpr (std::is_same_v<void, T>)
        {
          co_awaiter_.await_resume();
          return void_result{};
        }
        else
        {
          return co_awaiter_.await_resume();
        }
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        switch (result_state_)
        {
          case result_state::none:
          case result_state::has_stopped:
            std::terminate();
          case result_state::has_result:
            return co_awaiter_.get_result_exception();
          case result_state::has_timeout:
            return {};
        }
      }

      void start() noexcept
      {
        pending_count_ = 1;
        init_parent_cancellation_callback();

        start_chains();

        --pending_count_;
        if (0 != pending_count_)
        {
          return;
        }

        parent_stop_cb_.reset();

        if (result_state::has_stopped == result_state_)
        {
          parent_ctx_.invoke_stopped();
          return;
        }

        parent_ctx_.invoke_result_ready();
      }

    private:
      void on_shared_continue() noexcept
      {
        parent_stop_cb_.reset();

        if (result_state::has_stopped == result_state_)
        {
          parent_ctx_.invoke_stopped();
          return;
        }

        if (parent_handle_)
        {
          parent_handle_.resume();
          return;
        }

        parent_ctx_.invoke_result_ready();
      }

      void on_task_result_ready() noexcept
      {
        if ((result_state::none == result_state_) ||
            (result_state::has_timeout == result_state_))
        {
          result_state_ = result_state::has_result;
          children_stop_source_.request_stop();
        }

        --pending_count_;
        if (0 != pending_count_)
        {
          return;
        }
        on_shared_continue();
      }

      void on_task_stopped() noexcept
      {
        if (!children_stop_source_.stop_requested())
        {
          result_state_ = result_state::has_stopped;
          // stop the timer this indirect way
          // to handle the case where the timer was not started
          children_stop_source_.request_stop();
        }

        --pending_count_;
        if (0 != pending_count_)
        {
          return;
        }
        on_shared_continue();
      }

      void init_parent_cancellation_callback() noexcept
      {
        parent_stop_cb_.emplace(
          parent_ctx_.get_stop_token(),
          make_member_callback<&awaiter::on_parent_cancel>(this));
      }

      void on_parent_cancel() noexcept
      {
        parent_stop_cb_.reset();
        result_state_ = result_state::has_stopped;
        children_stop_source_.request_stop();
      }

      void start_chains() noexcept
      {
        assert(1 == pending_count_);
        pending_count_ = 2;
        co_awaiter_.start();

        if (1 == pending_count_)
        {
          // co_awaiter_ completed early, no need to sleep
          return;
        }

        ++pending_count_;
        schedule_timer();
      }

      void schedule_timer() noexcept
      {
        timer_node_.cb = make_member_callback<&awaiter::on_timer>(this);
        parent_ctx_.insert_timer_node(timer_node_);
        timer_stop_cb_.emplace(
          children_stop_source_.get_token(),
          make_member_callback<&awaiter::on_timer_cancel>(this));
      }

      void on_timer() noexcept
      {
        timer_stop_cb_.reset();

        if (result_state::none == result_state_)
        {
          result_state_ = result_state::has_timeout;
          children_stop_source_.request_stop();
        }

        --pending_count_;
        if (0 != pending_count_)
        {
          return;
        }

        parent_stop_cb_.reset();

        // this is called from the run loop
        // invoke rather than schedule
        if (result_state::has_stopped == result_state_)
        {
          parent_ctx_.invoke_stopped();
          return;
        }

        if (parent_handle_)
        {
          parent_handle_.resume();
          return;
        }

        parent_ctx_.invoke_result_ready();
      }

      void on_timer_cancel() noexcept
      {
        timer_stop_cb_.reset();
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
        CoTask& co_task,
        std::chrono::steady_clock::time_point deadline
      ) noexcept:
        co_work_{ co_task.get_work() },
        deadline_{ deadline }
      {
      }

      work(const work&) = delete;
      work& operator=(const work&) = delete;
      work(work&&) noexcept = default;
      work& operator=(work&&) noexcept = default;

      [[nodiscard]] awaiter get_awaiter(context& ctx)
      {
        return {ctx, co_work_, deadline_};
      }
    };

  private:
    work work_;

  public:
    wait_for_task(
      CoTask& co_task,
      std::chrono::steady_clock::time_point deadline
    ) noexcept :
      work_{ co_task, deadline }
    {
    }

    wait_for_task(const wait_for_task&) = delete;
    wait_for_task& operator=(const wait_for_task&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return std::move(work_);
    }
  };

  template<is_co_task CoTask>
  [[nodiscard]] wait_for_task<CoTask>
    async_wait_for(CoTask co_task, std::chrono::steady_clock::duration sleep_duration)
  {
    return wait_for_task<CoTask>{ co_task, std::chrono::steady_clock::now() + sleep_duration };
  }
}