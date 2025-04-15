#pragma once

// #include "callback.h"
// #include "context.h"
// #include "coro_type_traits.h"
// #include "stop_util.h"
// #include "wait_any_type_traits.h"

#include <chrono>
#include <coroutine>
#include <type_traits>
#include <optional>

namespace coro_st
{
  namespace impl
  {
    template<typename T>
    struct wait_any_awaiter_shared_data
    {
      using ResultType = wait_any_result<T>;

      context& parent_ctx_;
      std::coroutine_handle<> parent_handle_;
      stop_source wait_stop_source_;
      std::optional<stop_callback<callback>> stop_cb_;
      size_t pending_count_;
      std::variant<std::monostate, ResultType, std::exception_ptr> result_;

      wait_any_awaiter_shared_data(context& parent_ctx) noexcept :
        parent_ctx_{ parent_ctx },
        parent_handle_{},
        wait_stop_source_{},
        stop_cb_{},
        pending_count_{ 0 },
        result_{}
      {
      }

      wait_any_awaiter_shared_data(const wait_any_awaiter_shared_data&) = delete;
      wait_any_awaiter_shared_data& operator=(const wait_any_awaiter_shared_data&) = delete;

      void init_cancellation_callback() noexcept
      {
        stop_cb_.emplace(
          parent_ctx_.get_stop_token(),
          make_member_callback<&wait_any_awaiter_shared_data::on_cancel>(this));
      }

      void on_continue() noexcept
      {
        stop_cb_ = std::nullopt;

        if (parent_ctx_.get_stop_token().stop_requested())
        {
          parent_ctx_.schedule_cancellation_callback();
          return;
        }

        if (parent_handle_)
        {
          ready_node& n = parent_ctx_.get_chain_node();
          n.cb = make_resume_coroutine_callback(parent_handle_);
          parent_ctx_.push_ready_node(n);
          return;
        }

        parent_ctx_.schedule_continuation_callback();
      }

      void on_cancel() noexcept
      {
        stop_cb_ = std::nullopt;
        wait_stop_source_.request_stop();
      }
    };

    template<typename SharedData, is_co_work CoWork>
    struct wait_any_awaiter_chain_data
    {
      wait_any_awaiter_chain_data(SharedData& shared_data, size_t index, CoWork& co_work) noexcept :
        chain_ctx_{
          shared_data_.wait_stop_source_.get_token(),
          make_member_callback<&wait_any_awaiter_chain_data::on_continue>(this),
          make_member_callback<&wait_any_awaiter_chain_data::on_cancel>(this),
          },
        ctx_{ shared_data_.parent_ctx_, chain_ctx_ },
        co_awaiter_{ co_work.get_awaiter(ctx_) }
      {
      }

      wait_any_awaiter_chain_data(const wait_any_awaiter_chain_data&) = delete;
      wait_any_awaiter_chain_data& operator=(const wait_any_awaiter_chain_data&) = delete;

      void on_continue() noexcept
      {
        if (!shared_data_.parent_ctx_.get_stop_token().stop_requested())
        {
          if (0 == shared_data_.result_.index())
          {
            std::exception_ptr e = co_awaiter_.get_result_exception();
            if (e)
            {
              shared_data_.result_.template emplace<2>(e);
            }
            else
            {
              try
              {
                if constexpr (std::is_same_v<void, co_work_result_t<CoWork>>)
                {
                  co_awaiter_.await_resume();
                  shared_data_.result_.template emplace<1>(index_);
                }
                else
                {
                  shared_data_.result_.template emplace<1>(index_, co_awaiter_.await_resume());
                }
              }
              catch(...)
              {
                shared_data_.result_.template emplace<2>(std::current_exception());
              }
            }

            shared_data_.wait_stop_source_.request_stop();
          }
        }

        --shared_data_.pending_count_;
        if (0 != shared_data_.pending_count_)
        {
          return;
        }
        shared_data_.on_continue();
      }

      void on_cancel() noexcept
      {
        --shared_data_.pending_count_;
        if (0 != shared_data_.pending_count_)
        {
          return;
        }
        shared_data_.on_continue();
      }
    };
  }

  template<is_co_task CoTask>
  class [[nodiscard]] with_timer_task
  {
    using T = co_task_result_t<CoTask>;
    using ResultType = std::conditional_t<
      std::is_same_v<void, T>, bool, T>;

    co_task_work_t<CoTask> co_work_;
    std::chrono::steady_clock::time_point deadline_;

  public:
    with_timer_task(
      CoTask& co_task,
      std::chrono::steady_clock::time_point deadline
    ) noexcept :
      co_work_{ co_task.get_work() },
      deadline_{ deadline }
    {
    }

    with_timer_task(const with_timer_task&) = delete;
    with_timer_task& operator=(const with_timer_task&) = delete;

  private:
    class [[nodiscard]] awaiter
    {
      context& parent_ctx_;
      std::coroutine_handle<> parent_handle_;
      stop_source with_timer_stop_source_;
      std::optional<stop_callback<callback>> overall_stop_cb_;
      bool has_result{ false };
      bool task_completed_{ false };
      bool timer_completed_{ false };

      chain_context task_chain_ctx_;
      context task_ctx_;
      co_task_awaiter_t<CoTask> co_awaiter_;

      chain_context timer_chain_ctx_;
      context timer_ctx_;
      timer_node timer_node_;
      std::optional<stop_callback<callback>> timer_stop_cb_;

    public:
      awaiter(
        context& parent_ctx,
        co_task_work_t<CoTask>& co_work,
        std::chrono::steady_clock::time_point deadline
      ) noexcept :
        parent_ctx_{ parent_ctx },
        parent_handle_{},
      wait_stop_source_{},
      stop_cb_{},
        chain_data_{(
            impl::wait_any_awaiter_chain_data_tuple_builder{shared_data_, I, std::get<I>(co_works_tuple)})... }
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
        shared_data_.parent_handle_ = handle;

        shared_data_.pending_count_ = N + 1;
        shared_data_.init_cancellation_callback();

        start_chains();

        --shared_data_.pending_count_;
        if (0 != shared_data_.pending_count_)
        {
          return true;
        }

        if (shared_data_.parent_ctx_.get_stop_token().stop_requested())
        {
          shared_data_.parent_ctx_.schedule_cancellation_callback();
          return true;
        }

        return false;
      }

      ResultType await_resume() const
      {
        switch(shared_data_.result_.index())
        {
          case 1:
            return std::move(std::get<1>(shared_data_.result_));
          case 2:
            std::rethrow_exception(std::get<2>(shared_data_.result_));
          default:
            std::terminate();
        }
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        if (2 != shared_data_.result_.index())
        {
          return {};
        }
        return std::get<2>(shared_data_.result_);
      }

      void start_as_chain_root() noexcept
      {
        shared_data_.pending_count_ = N + 1;
        shared_data_.init_cancellation_callback();

        start_chains();

        --shared_data_.pending_count_;
        if (0 != shared_data_.pending_count_)
        {
          return;
        }

        if (shared_data_.parent_ctx_.get_stop_token().stop_requested())
        {
          shared_data_.parent_ctx_.schedule_cancellation_callback();
          return;
        }

        shared_data_.on_continue();
      }


    private:
      void start_chains() noexcept
      {
        std::apply (
          [](auto &... chain) {
            (chain.co_awaiter_.start_as_chain_root(),...);
          },
          chain_data_
        );
      }
    };

    struct [[nodiscard]] work
    {
      co_task_work_t<CoTask> co_work_;
      std::chrono::steady_clock::time_point deadline_;

      work(
        co_task_work_t<CoTask>&& co_work_,
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