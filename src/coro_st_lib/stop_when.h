#pragma once

#include "callback.h"
#include "context.h"
#include "coro_type_traits.h"
#include "stop_util.h"
#include "value_type_traits.h"

#include <cassert>
#include <coroutine>
#include <optional>

namespace coro_st
{
  template<is_co_task CoTask1, is_co_task CoTask2>
  class [[nodiscard]] stop_when_task
  {
    using CoWork1 = co_task_work_t<CoTask1>;
    using CoWork2 = co_task_work_t<CoTask2>;
    using CoAwaiter1 = co_task_awaiter_t<CoTask1>;
    using CoAwaiter2 = co_task_awaiter_t<CoTask2>;
    using T = co_task_result_t<CoTask1>;
    using ResultType = std::optional<value_type_traits::value_type_t<T>>;

    class [[nodiscard]] awaiter
    {
      enum class result_state
      {
        none,
        has_result1,
        has_stopped1,
        has_error2,
        has_stopped2,
      };

      context& parent_ctx_;
      std::coroutine_handle<> parent_handle_;
      std::optional<stop_callback<callback>> parent_stop_cb_;
      stop_source children_stop_source_;
      size_t pending_count_{ 0 };
      result_state result_state_{ result_state::none };

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
        task_ctx1_{
          parent_ctx_,
          children_stop_source_.get_token(),
          make_member_completion<
            &awaiter::on_task1_result_ready,
            &awaiter::on_task1_stopped
            >(this)
        },
        co_awaiter1_{ co_work1.get_awaiter(task_ctx1_) },
        task_ctx2_{
          parent_ctx_,
          children_stop_source_.get_token(),
          make_member_completion<
            &awaiter::on_task2_result_ready,
            &awaiter::on_task2_stopped
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

        pending_count_ = 1;
        init_parent_cancellation_callback();

        start_chains();

        --pending_count_;
        if (0 != pending_count_)
        {
          return true;
        }

        parent_stop_cb_.reset();

        if (result_state::has_stopped1 == result_state_)
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
          case result_state::has_stopped1:
            std::terminate();
          case result_state::has_result1:
            if constexpr (std::is_same_v<void, T>)
            {
              co_awaiter1_.await_resume();
              return void_result{};
            }
            else
            {
              return co_awaiter1_.await_resume();
            }
          case result_state::has_error2:
            std::rethrow_exception(co_awaiter2_.get_result_exception());
          case result_state::has_stopped2:
            break;
        }
        return std::nullopt;
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        switch (result_state_)
        {
          case result_state::none:
          case result_state::has_stopped1:
            std::terminate();
          case result_state::has_result1:
            return co_awaiter1_.get_result_exception();
          case result_state::has_error2:
            return co_awaiter2_.get_result_exception();
          case result_state::has_stopped2:
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

        if (result_state::has_stopped1 == result_state_)
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

        if (result_state::has_stopped1 == result_state_)
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
        if ((result_state::none == result_state_) ||
            (result_state::has_stopped2 == result_state_))
        {
          result_state_ = result_state::has_result1;
          children_stop_source_.request_stop();
        }

        --pending_count_;
        if (0 != pending_count_)
        {
          return;
        }
        on_shared_continue();
      }

      void on_task1_stopped() noexcept
      {
        if (!children_stop_source_.stop_requested())
        {
          result_state_ = result_state::has_stopped1;
          // stop task2 this indirect way
          // to handle the case where the task2 was not started
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
          if (co_awaiter2_.get_result_exception())
          {
            result_state_ = result_state::has_error2;
          }
          else
          {
            result_state_ = result_state::has_stopped2;
          }
          children_stop_source_.request_stop();
        }

        --pending_count_;
        if (0 != pending_count_)
        {
          return;
        }
        on_shared_continue();
      }

      void on_task2_stopped() noexcept
      {
        if (!children_stop_source_.stop_requested())
        {
          result_state_ = result_state::has_stopped2;
          // stop task2 this indirect way
          // to handle the case where the task2 was not started
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
        result_state_ = result_state::has_stopped1;
        children_stop_source_.request_stop();
      }

      void start_chains() noexcept
      {
        assert(1 == pending_count_);
        pending_count_ = 2;
        co_awaiter1_.start();

        if (1 == pending_count_)
        {
          // co_awaiter_ completed early, no need to sleep
          return;
        }

        ++pending_count_;
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
    stop_when_task(
      CoTask1& co_task1,
      CoTask2& co_task2
    ) noexcept :
      work_{ co_task1, co_task2 }
    {
    }

    stop_when_task(const stop_when_task&) = delete;
    stop_when_task& operator=(const stop_when_task&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return std::move(work_);
    }
  };

  template<is_co_task CoTask1, is_co_task CoTask2>
  [[nodiscard]] stop_when_task<CoTask1, CoTask2>
    async_stop_when(CoTask1 co_task1, CoTask2 co_task2)
  {
    return {co_task1, co_task2};
  }
}