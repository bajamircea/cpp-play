#pragma once

#include "callback.h"
#include "context.h"
#include "coro_type_traits.h"
#include "stop_util.h"
#include "value_type_traits.h"

#include <coroutine>
#include <optional>
#include <tuple>

namespace coro_st
{
  namespace impl
  {
    struct wait_all_awaiter_shared_data
    {
      context& parent_ctx_;
      std::coroutine_handle<> parent_handle_;
      stop_source wait_stop_source_;
      std::optional<stop_callback<callback>> stop_cb_;
      size_t pending_count_;
      std::exception_ptr exception_;
      bool stopped_;

      wait_all_awaiter_shared_data(context& parent_ctx) noexcept :
        parent_ctx_{ parent_ctx },
        parent_handle_{},
        wait_stop_source_{},
        stop_cb_{},
        pending_count_{ 0 },
        exception_{},
        stopped_{ false }
      {
      }

      wait_all_awaiter_shared_data(const wait_all_awaiter_shared_data&) = delete;
      wait_all_awaiter_shared_data& operator=(const wait_all_awaiter_shared_data&) = delete;

      void init_parent_cancellation_callback() noexcept
      {
        stop_cb_.emplace(
          parent_ctx_.get_stop_token(),
          make_member_callback<&wait_all_awaiter_shared_data::on_parent_cancel>(this));
      }

      void on_shared_continue() noexcept
      {
        stop_cb_.reset();

        if (parent_ctx_.get_stop_token().stop_requested() || stopped_)
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

      void on_parent_cancel() noexcept
      {
        stop_cb_.reset();
        wait_stop_source_.request_stop();
      }
    };

    template<is_co_work CoWork>
    struct wait_all_awaiter_chain_data
    {
      wait_all_awaiter_shared_data& shared_data_;
      chain_context chain_ctx_;
      context ctx_;
      co_work_awaiter_t<CoWork> co_awaiter_;

      wait_all_awaiter_chain_data(
        wait_all_awaiter_shared_data& shared_data,
        CoWork& co_work
      ) :
        shared_data_{ shared_data },
        chain_ctx_{
          shared_data_.wait_stop_source_.get_token(),
          make_member_callback<&wait_all_awaiter_chain_data::on_continue>(this),
          make_member_callback<&wait_all_awaiter_chain_data::on_cancel>(this),
          },
        ctx_{ shared_data_.parent_ctx_, chain_ctx_ },
        co_awaiter_{ co_work.get_awaiter(ctx_) }
      {
      }

      wait_all_awaiter_chain_data(const wait_all_awaiter_chain_data&) = delete;
      wait_all_awaiter_chain_data& operator=(const wait_all_awaiter_chain_data&) = delete;

      void on_continue() noexcept
      {
        if (!shared_data_.parent_ctx_.get_stop_token().stop_requested())
        {
          if (!shared_data_.exception_ && !shared_data_.stopped_)
          {
            std::exception_ptr e = co_awaiter_.get_result_exception();
            if (e)
            {
              shared_data_.exception_ = e;
              shared_data_.wait_stop_source_.request_stop();
            }
          }
        }

        --shared_data_.pending_count_;
        if (0 != shared_data_.pending_count_)
        {
          return;
        }
        shared_data_.on_shared_continue();
      }

      void on_cancel() noexcept
      {
        if (!shared_data_.parent_ctx_.get_stop_token().stop_requested())
        {
          if (!shared_data_.wait_stop_source_.stop_requested())
          {
            shared_data_.stopped_ = true;
            shared_data_.wait_stop_source_.request_stop();
          }
        }

        --shared_data_.pending_count_;
        if (0 != shared_data_.pending_count_)
        {
          return;
        }
        shared_data_.on_shared_continue();
      }

      auto get_result() const
      {
        if constexpr (std::is_same_v<void, co_work_result_t<CoWork>>)
        {
          co_awaiter_.await_resume();
          return void_result{};
        }
        else
        {
          return co_awaiter_.await_resume();
        }
      }
    };

    template<is_co_work CoWork>
    class wait_all_awaiter_chain_data_tuple_builder
    {
      wait_all_awaiter_shared_data* shared_data_;
      CoWork* co_work_;

      public:
      wait_all_awaiter_chain_data_tuple_builder(
        wait_all_awaiter_shared_data& shared_data,
        CoWork& co_work
      ) noexcept :
        shared_data_{&shared_data},
        co_work_{&co_work}
      {
      }

      wait_all_awaiter_chain_data_tuple_builder(const wait_all_awaiter_chain_data_tuple_builder&) = delete;
      wait_all_awaiter_chain_data_tuple_builder& operator=(const wait_all_awaiter_chain_data_tuple_builder&) = delete;

      operator wait_all_awaiter_chain_data<CoWork>() const
      {
        return {*shared_data_, *co_work_};
      }
    };
  }

  template<is_co_task... CoTasks>
  class [[nodiscard]] wait_all_task
  {
    static constexpr size_t N = sizeof... (CoTasks);
    using WorksTuple = std::tuple<co_task_work_t<CoTasks>...>;
    using WorksTupleSeq = std::index_sequence_for<CoTasks...>;
    using ResultType = std::tuple<
      value_type_traits::value_type_t<
        co_task_result_t<CoTasks>>...>;

    class [[nodiscard]] awaiter
    {
      using ChainDataTuple =
        std::tuple<
          impl::wait_all_awaiter_chain_data<co_task_work_t<CoTasks>>...>;

      impl::wait_all_awaiter_shared_data shared_data_;
      ChainDataTuple chain_data_;

    public:
      template<std::size_t... I>
      awaiter(
        std::index_sequence<I...>,
        context& parent_ctx,
        WorksTuple& co_works_tuple
      ) :
        shared_data_{ parent_ctx },
        chain_data_{(
            impl::wait_all_awaiter_chain_data_tuple_builder{shared_data_, std::get<I>(co_works_tuple)})... }
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
        shared_data_.init_parent_cancellation_callback();

        start_chains();

        --shared_data_.pending_count_;
        if (0 != shared_data_.pending_count_)
        {
          return true;
        }

        shared_data_.stop_cb_.reset();

        if (shared_data_.parent_ctx_.get_stop_token().stop_requested())
        {
          shared_data_.parent_ctx_.schedule_cancellation_callback();
          return true;
        }

        return false;
      }

      ResultType await_resume() const
      {
        if (shared_data_.exception_)
        {
          std::rethrow_exception(shared_data_.exception_);
        }

        return std::apply(
          [](auto &... chain) -> ResultType {
            return std::make_tuple(chain.get_result()...);
          },
          chain_data_
        );
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        return shared_data_.exception_;
      }

      void start_as_chain_root() noexcept
      {
        shared_data_.pending_count_ = N + 1;
        shared_data_.init_parent_cancellation_callback();

        start_chains();

        --shared_data_.pending_count_;
        if (0 != shared_data_.pending_count_)
        {
          return;
        }

        shared_data_.stop_cb_.reset();

        if (shared_data_.parent_ctx_.get_stop_token().stop_requested())
        {
          shared_data_.parent_ctx_.schedule_cancellation_callback();
          return;
        }

        callback continuation_cb = shared_data_.parent_ctx_.get_continuation_callback();
        continuation_cb.invoke();
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
      WorksTuple co_works_tuple_;

      work(CoTasks&... co_tasks) noexcept:
        co_works_tuple_{ co_tasks.get_work()... }
      {
      }

      work(const work&) = delete;
      work& operator=(const work&) = delete;
      work(work&&) noexcept = default;
      work& operator=(work&&) noexcept = default;

      [[nodiscard]] awaiter get_awaiter(context& ctx)
      {
        return awaiter(WorksTupleSeq{}, ctx, co_works_tuple_);
      }
    };

  private:
    work work_;

  public:
    wait_all_task(CoTasks&... co_tasks) noexcept :
      work_{ co_tasks... }
    {
    }

    wait_all_task(const wait_all_task&) = delete;
    wait_all_task& operator=(const wait_all_task&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return std::move(work_);
    }
  };

  template<is_co_task... CoTasks>
  [[nodiscard]] wait_all_task<CoTasks...>
    async_wait_all(CoTasks... co_tasks) noexcept
      requires(sizeof... (CoTasks) > 1)
  {
    return wait_all_task<CoTasks...>{ co_tasks... };
  }
}