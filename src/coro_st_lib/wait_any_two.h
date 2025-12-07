#pragma once

  // use wait_any, this largely serves learning purposes

#include "callback.h"
#include "context.h"
#include "coro_type_traits.h"
#include "stop_util.h"

#include <cassert>
#include <coroutine>
#include <type_traits>

namespace coro_st
{
  template<is_co_task CoTask1, is_co_task CoTask2>
  class [[nodiscard]] wait_any_two_task
  {
    using CoWork1 = co_task_work_t<CoTask1>;
    using CoWork2 = co_task_work_t<CoTask2>;
    using CoAwaiter1 = co_task_awaiter_t<CoTask1>;
    using CoAwaiter2 = co_task_awaiter_t<CoTask2>;
    using ResultType = co_task_result_t<CoTask1>;

    class [[nodiscard]] awaiter
    {
      enum class result_state
      {
        none,
        has_value_or_error1,
        has_value_or_error2,
        has_stopped,
      };

      context& parent_ctx_;
      std::coroutine_handle<> parent_handle_;
      std::optional<stop_callback<callback>> parent_stop_cb_;
      stop_source children_stop_source_;
      size_t pending_count_{ 0 };
      result_state result_state_{ result_state::none };
      std::exception_ptr exception_{};

      context task_ctx1_;
      CoAwaiter1 co_awaiter1_;

      context task_ctx2_;
      CoAwaiter2 co_awaiter2_;

    public:
      awaiter(
        context& parent_ctx,
        CoWork1& co_work1,
        CoWork2& co_work2
      ) :
        parent_ctx_{ parent_ctx },
        parent_handle_{},
        parent_stop_cb_{},
        children_stop_source_{},
        pending_count_{ 0 },
        result_state_{ result_state::none },
        exception_{},
        task_ctx1_{
          parent_ctx_,
          children_stop_source_.get_token(),
          make_member_completion<
            &awaiter::on_task1_result_ready,
            &awaiter::on_task_stopped
            >(this)
        },
        co_awaiter1_{ co_work1.get_awaiter(task_ctx1_) },
        task_ctx2_{
          parent_ctx_,
          children_stop_source_.get_token(),
          make_member_completion<
            &awaiter::on_task2_result_ready,
            &awaiter::on_task_stopped
            >(this)
        },
        co_awaiter2_{ co_work2.get_awaiter(task_ctx2_) }
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

        // two children will be running in addition to this function
        pending_count_ = 2 + 1;
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
          default:
          case result_state::none:
          case result_state::has_stopped:
            std::terminate();
          case result_state::has_value_or_error1:
            return co_awaiter1_.await_resume();
          case result_state::has_value_or_error2:
            return co_awaiter2_.await_resume();
        }
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        switch (result_state_)
        {
          case result_state::none:
          case result_state::has_stopped:
            std::terminate();
          case result_state::has_value_or_error1:
            return co_awaiter1_.get_result_exception();
          case result_state::has_value_or_error2:
            return co_awaiter2_.get_result_exception();
        }
      }

      void start() noexcept
      {
        // two children will be running in addition to this function
        pending_count_ = 2 + 1;
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

      void on_task1_result_ready() noexcept
      {
        if (result_state::none == result_state_)
        {
          result_state_ = result_state::has_value_or_error1;
          children_stop_source_.request_stop();
        }

        --pending_count_;
        if (0 != pending_count_)
        {
          return;
        }
        on_shared_continue();
      }

      void on_task2_result_ready() noexcept
      {
        if (result_state::none == result_state_)
        {
          result_state_ = result_state::has_value_or_error2;
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
        co_awaiter1_.start();
        co_awaiter2_.start();
      }
    };

    struct [[nodiscard]] work
    {
      CoWork1 co_work1_;
      CoWork2 co_work2_;

      work(
        CoTask1& co_task1,
        CoTask2& co_task2
      ) noexcept:
        co_work1_{ co_task1.get_work() },
        co_work2_{ co_task2.get_work() }
      {
      }

      work(const work&) = delete;
      work& operator=(const work&) = delete;
      work(work&&) noexcept = default;
      work& operator=(work&&) noexcept = default;

      [[nodiscard]] awaiter get_awaiter(context& ctx)
      {
        return {ctx, co_work1_, co_work2_};
      }
    };

  private:
    work work_;

  public:
    wait_any_two_task(
      CoTask1& co_task1,
      CoTask2& co_task2
    ) noexcept :
      work_{ co_task1, co_task2 }
    {
    }

    wait_any_two_task(const wait_any_two_task&) = delete;
    wait_any_two_task& operator=(const wait_any_two_task&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return std::move(work_);
    }
  };

  template<is_co_task CoTask1, is_co_task CoTask2>
  [[nodiscard]] wait_any_two_task<CoTask1, CoTask2>
    async_wait_any_two(CoTask1 co_task1, CoTask2 co_task2)
  {
    static_assert(std::is_same_v<co_task_result_t<CoTask1>, co_task_result_t<CoTask2>>);
    return {co_task1, co_task2};
  }
}