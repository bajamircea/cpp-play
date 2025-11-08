#pragma once

#include "callback.h"
#include "context.h"
#include "coro_type_traits.h"
#include "stop_util.h"

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

    class [[nodiscard]] awaiter
    {
      enum class result_state
      {
        none,
        has_value_or_error1,
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
        task_ctx1_{ children_stop_source_.get_token() },
        co_awaiter1_{ co_work1.get_awaiter(task_ctx1_) },
        task_ctx2_{ children_stop_source_.get_token() },
        co_awaiter2_{ co_work2.get_awaiter(task_ctx2_) }
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
      }

      void await_resume()
      {
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        return {};
      }

      void start_as_chain_root() noexcept
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
          parent_ctx_.invoke_cancellation();
          return;
        }

        parent_ctx_.invoke_continuation();
      }

    private:

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
        co_awaiter1_.start_as_chain_root();

        if (1 == pending_count_)
        {
          // co_awaiter_ completed early, no need to sleep
          return;
        }

        ++pending_count_;
        co_awaiter2_.start_as_chain_root();
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