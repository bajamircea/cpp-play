#pragma once

#include "context.h"
#include "coro_type_traits.h"
#include "stop_util.h"

#include <cassert>
#include <coroutine>

namespace coro_st
{
  template<typename T, is_co_task CoTask>
  class [[nodiscard]] cast_task
  {
    using CoWork = co_task_work_t<CoTask>;
    using CoAwaiter = co_task_awaiter_t<CoTask>;

    class [[nodiscard]] awaiter
    {
      enum class result_state
      {
        none,
        has_result,
        has_stopped,
      };

      context& parent_ctx_;
      std::coroutine_handle<> parent_handle_;
      bool pending_start_{ false };
      result_state result_state_{ result_state::none };

      context task_ctx_;
      CoAwaiter co_awaiter_;

    public:
      awaiter(
        context& parent_ctx,
        CoWork& co_work
      ) :
        parent_ctx_{ parent_ctx },
        parent_handle_{},
        pending_start_{ false },
        result_state_{ result_state::none },
        task_ctx_{
          parent_ctx_,
          parent_ctx_.get_stop_token(),
          make_member_completion<
            &awaiter::on_task_result_ready,
            &awaiter::on_task_stopped
            >(this)
          },
        co_awaiter_{ co_work.get_awaiter(task_ctx_) }
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

        pending_start_ = true;
        co_awaiter_.start();
        pending_start_ = false;

        if (result_state::none == result_state_)
        {
          return true;
        }

        if (result_state::has_stopped == result_state_)
        {
          parent_ctx_.invoke_stopped();
          return true;
        }

        return false;
      }

      T await_resume()
      {
        if constexpr (std::is_same_v<void, T>)
        {
          co_awaiter_.await_resume();
          return;
        }
        else
        {
          return co_awaiter_.await_resume();
        }
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        return co_awaiter_.get_result_exception();
      }

      void start() noexcept
      {
        pending_start_ = true;
        co_awaiter_.start();
        pending_start_ = false;

        if (result_state::none == result_state_)
        {
          return;
        }

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
        if (pending_start_)
        {
          return;
        }

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
        if (result_state::none == result_state_)
        {
          result_state_ = result_state::has_result;
        }
        on_shared_continue();
      }

      void on_task_stopped() noexcept
      {
        result_state_ = result_state::has_stopped;
        on_shared_continue();
      }
    };

    struct [[nodiscard]] work
    {
      CoWork co_work_;

      work(CoTask& co_task) noexcept:
        co_work_{ co_task.get_work() }
      {
      }

      work(const work&) = delete;
      work& operator=(const work&) = delete;
      work(work&&) noexcept = default;
      work& operator=(work&&) noexcept = default;

      [[nodiscard]] awaiter get_awaiter(context& ctx)
      {
        return {ctx, co_work_};
      }
    };

  private:
    work work_;

  public:
    cast_task(CoTask& co_task) noexcept :
      work_{ co_task }
    {
    }

    cast_task(const cast_task&) = delete;
    cast_task& operator=(const cast_task&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return std::move(work_);
    }
  };

  template<typename T, is_co_task CoTask>
  [[nodiscard]] cast_task<T, CoTask>
    async_cast(CoTask co_task)
  {
    return cast_task<T, CoTask>{ co_task };
  }
}