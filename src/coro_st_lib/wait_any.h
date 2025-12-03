#pragma once

#include "callback.h"
#include "context.h"
#include "coro_type_traits.h"
#include "stop_util.h"
#include "wait_any_type_traits.h"

#include <coroutine>
#include <optional>
#include <tuple>
#include <type_traits>
#include <variant>

namespace coro_st
{
  template<typename T>
  struct wait_any_result
  {
    size_t index{};
    T value{};
  };

  template<>
  struct wait_any_result<void>
  {
    size_t index{};
  };

  namespace impl
  {
    static constexpr size_t g_none_result_type = 0;
    static constexpr size_t g_value_result_type = 1;
    static constexpr size_t g_error_result_type = 2;
    static constexpr size_t g_stopped_result_type = 3;

    template<typename T>
    struct wait_any_awaiter_shared_data
    {
      using ResultType = wait_any_result<T>;

      context& parent_ctx_;
      std::coroutine_handle<> parent_handle_;
      std::optional<stop_callback<callback>> parent_stop_cb_;
      stop_source children_stop_source_;
      size_t pending_count_;
      // see g_..._result_type above
      std::variant<std::monostate, ResultType, std::exception_ptr, std::monostate> result_;

      wait_any_awaiter_shared_data(context& parent_ctx) noexcept :
        parent_ctx_{ parent_ctx },
        parent_handle_{},
        parent_stop_cb_{},
        children_stop_source_{},
        pending_count_{ 0 },
        result_{}
      {
      }

      wait_any_awaiter_shared_data(const wait_any_awaiter_shared_data&) = delete;
      wait_any_awaiter_shared_data& operator=(const wait_any_awaiter_shared_data&) = delete;

      void init_parent_cancellation_callback() noexcept
      {
        parent_stop_cb_.emplace(
          parent_ctx_.get_stop_token(),
          make_member_callback<&wait_any_awaiter_shared_data::on_parent_cancel>(this));
      }

      void on_shared_continue() noexcept
      {
        parent_stop_cb_.reset();

        if (g_stopped_result_type == result_.index())
        {
          parent_ctx_.schedule_cancellation();
          return;
        }

        if (parent_handle_)
        {
          parent_ctx_.schedule_coroutine_resume(parent_handle_);
          return;
        }

        parent_ctx_.schedule_continuation();
      }

      void on_parent_cancel() noexcept
      {
        parent_stop_cb_.reset();
        result_.template emplace<g_stopped_result_type>();
        children_stop_source_.request_stop();
      }
    };

    template<typename SharedData, is_co_work CoWork>
    struct wait_any_awaiter_chain_data
    {
      SharedData& shared_data_;
      size_t index_;
      context ctx_;
      co_work_awaiter_t<CoWork> co_awaiter_;

      wait_any_awaiter_chain_data(
        SharedData& shared_data,
        size_t index,
        CoWork& co_work
      ) :
        shared_data_{ shared_data },
        index_{ index },
        ctx_{
          shared_data_.parent_ctx_,
          shared_data_.children_stop_source_.get_token(),
          make_member_completion<
            &wait_any_awaiter_chain_data::on_result_ready,
            &wait_any_awaiter_chain_data::on_stopped
            >(this)
        },
        co_awaiter_{ co_work.get_awaiter(ctx_) }
      {
      }

      wait_any_awaiter_chain_data(const wait_any_awaiter_chain_data&) = delete;
      wait_any_awaiter_chain_data& operator=(const wait_any_awaiter_chain_data&) = delete;

      void on_result_ready() noexcept
      {
        if (g_none_result_type == shared_data_.result_.index())
        {
          std::exception_ptr e = co_awaiter_.get_result_exception();
          if (e)
          {
            shared_data_.result_.template emplace<g_error_result_type>(e);
          }
          else
          {
            try
            {
              if constexpr (std::is_same_v<void, co_work_result_t<CoWork>>)
              {
                co_awaiter_.await_resume();
                shared_data_.result_.template emplace<g_value_result_type>(index_);
              }
              else
              {
                shared_data_.result_.template emplace<g_value_result_type>(index_, co_awaiter_.await_resume());
              }
            }
            catch(...)
            {
              shared_data_.result_.template emplace<g_error_result_type>(std::current_exception());
            }
          }

          shared_data_.children_stop_source_.request_stop();
        }

        --shared_data_.pending_count_;
        if (0 != shared_data_.pending_count_)
        {
          return;
        }
        shared_data_.on_shared_continue();
      }

      void on_stopped() noexcept
      {
        if (g_none_result_type == shared_data_.result_.index())
        {
          shared_data_.result_.template emplace<g_stopped_result_type>();
          shared_data_.children_stop_source_.request_stop();
        }

        --shared_data_.pending_count_;
        if (0 != shared_data_.pending_count_)
        {
          return;
        }
        shared_data_.on_shared_continue();
      }
    };

    template<typename SharedData, is_co_work CoWork>
    class wait_any_awaiter_chain_data_tuple_builder
    {
      SharedData* shared_data_;
      size_t index_;
      CoWork* co_work_;

      public:
      wait_any_awaiter_chain_data_tuple_builder(
        SharedData& shared_data,
        size_t index,
        CoWork& co_work
      ) noexcept :
        shared_data_{&shared_data},
        index_{index},
        co_work_{&co_work}
      {
      }

      wait_any_awaiter_chain_data_tuple_builder(const wait_any_awaiter_chain_data_tuple_builder&) = delete;
      wait_any_awaiter_chain_data_tuple_builder& operator=(const wait_any_awaiter_chain_data_tuple_builder&) = delete;

      operator wait_any_awaiter_chain_data<SharedData, CoWork>() const
      {
        return {*shared_data_, index_, *co_work_};
      }
    };
  }

  template<is_co_task... CoTasks>
  class [[nodiscard]] wait_any_task
  {
    using T = wait_any_type_traits::value_type_t<
      co_task_result_t<CoTasks>...>;
    static constexpr size_t N = sizeof... (CoTasks);
    using WorksTuple = std::tuple<co_task_work_t<CoTasks>...>;
    using WorksTupleSeq = std::index_sequence_for<CoTasks...>;
    using ResultType = wait_any_result<T>;

    class [[nodiscard]] awaiter
    {
      using SharedData = impl::wait_any_awaiter_shared_data<T>;
      using ChainDataTuple =
        std::tuple<
          impl::wait_any_awaiter_chain_data<SharedData, co_task_work_t<CoTasks>>...>;

      SharedData shared_data_;
      ChainDataTuple chain_data_;

    public:
      template<std::size_t... I>
      awaiter(
        context& parent_ctx,
        std::index_sequence<I...>,
        WorksTuple& co_works_tuple
      ) :
        shared_data_{ parent_ctx },
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
        shared_data_.init_parent_cancellation_callback();

        start_chains();

        --shared_data_.pending_count_;
        if (0 != shared_data_.pending_count_)
        {
          return true;
        }

        shared_data_.parent_stop_cb_.reset();

        if (impl::g_stopped_result_type == shared_data_.result_.index())
        {
          shared_data_.parent_ctx_.schedule_cancellation();
          return true;
        }

        return false;
      }

      ResultType await_resume()
      {
        switch(shared_data_.result_.index())
        {
          case impl::g_value_result_type:
            return std::move(std::get<impl::g_value_result_type>(shared_data_.result_));
          case impl::g_error_result_type:
            std::rethrow_exception(std::get<impl::g_error_result_type>(shared_data_.result_));
          default:
            std::terminate();
        }
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        if (impl::g_error_result_type != shared_data_.result_.index())
        {
          return {};
        }
        return std::get<impl::g_error_result_type>(shared_data_.result_);
      }

      void start() noexcept
      {
        shared_data_.pending_count_ = N + 1;
        shared_data_.init_parent_cancellation_callback();

        start_chains();

        --shared_data_.pending_count_;
        if (0 != shared_data_.pending_count_)
        {
          return;
        }

        shared_data_.parent_stop_cb_.reset();

        if (impl::g_stopped_result_type == shared_data_.result_.index())
        {
          shared_data_.parent_ctx_.invoke_cancellation();
          return;
        }

        shared_data_.parent_ctx_.invoke_continuation();
      }

    private:
      void start_chains() noexcept
      {
        std::apply (
          [](auto &... chain) {
            (chain.co_awaiter_.start(),...);
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
        return {ctx, WorksTupleSeq{}, co_works_tuple_};
      }
    };

  private:
    work work_;

  public:
    wait_any_task(CoTasks&... co_tasks) noexcept :
      work_{ co_tasks... }
    {
    }

    wait_any_task(const wait_any_task&) = delete;
    wait_any_task& operator=(const wait_any_task&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return std::move(work_);
    }
  };

  template<is_co_task... CoTasks>
  [[nodiscard]] wait_any_task<CoTasks...>
    async_wait_any(CoTasks... co_tasks) noexcept
      requires(sizeof... (CoTasks) > 1)
  {
    return wait_any_task<CoTasks...>{ co_tasks... };
  }
}