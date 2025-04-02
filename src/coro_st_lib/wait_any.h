#pragma once

#include "context.h"
#include "coro_type_traits.h"
#include "stop_util.h"

#include <coroutine>
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
    template<typename T>
    struct wait_any_awaiter_shared_data
    {
      using ResultType = wait_any_result<T>;

      context& parent_ctx_;
      std::coroutine_handle<> parent_handle_;
      stop_source wait_stop_source_;
      size_t pending_count_;
      std::variant<std::monostate, ResultType, std::exception_ptr> result_;

      wait_any_awaiter_shared_data(context& parent_ctx) noexcept :
        parent_ctx_{ parent_ctx },
        parent_handle_{},
        wait_stop_source_{},
        pending_count_{ 0 },
        result_{}
      {
      }

      wait_any_awaiter_shared_data(const wait_any_awaiter_shared_data&) = delete;
      wait_any_awaiter_shared_data& operator=(const wait_any_awaiter_shared_data&) = delete;

      void on_done() noexcept
      {
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
    };

    template<typename SharedData, is_co_work CoWork>
    class wait_any_awaiter_chain_data
    {
      SharedData& shared_data_;
      size_t index_;
      chain_context chain_ctx_;
      context ctx_;
      co_work_awaiter_t<CoWork> co_awaiter_;
    public:
      wait_any_awaiter_chain_data(SharedData& shared_data, CoWork& co_work) noexcept :
        shared_data_{ shared_data },
        index_{ shared_data_.pending_count_++ },
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

            shared_data_.wait_stop_source_.request_stop();
          }
          --shared_data_.pending_count_;
          if (0 != shared_data_.pending_count_)
          {
            return;
          }
          shared_data_.on_done();
        }
      }

      void on_cancel() noexcept
      {
        --shared_data_.pending_count_;
        if (0 != shared_data_.pending_count_)
        {
          return;
        }
        shared_data_.on_done();
      }
    };

    template<typename SharedData, is_co_work CoWork>
    class wait_any_awaiter_chain_data_tuple_builder
    {
      SharedData* shared_data_;
      CoWork* co_work_;

      public:
      wait_any_awaiter_chain_data_tuple_builder(SharedData& shared_data, CoWork& co_work) noexcept :
        shared_data_{&shared_data},
        co_work_{&co_work}
      {
      }

      wait_any_awaiter_chain_data_tuple_builder(const wait_any_awaiter_chain_data_tuple_builder&) = delete;
      wait_any_awaiter_chain_data_tuple_builder& operator=(const wait_any_awaiter_chain_data_tuple_builder&) = delete;

      operator wait_any_awaiter_chain_data<SharedData, CoWork>() const
      {
        return {*shared_data_, *co_work_};
      }
    };
  }

  template<is_co_task... CoTasks>
  class [[nodiscard]] wait_any_task
  {
    using T = std::common_type_t<
      co_task_result_t<CoTasks>...>;
    static constexpr size_t N = sizeof... (CoTasks);
    using WorksTuple = std::tuple<co_task_work_t<CoTasks>...>;
    using WorksTupleSeq = std::index_sequence_for<CoTasks...>;
    using ResultType = wait_any_result<T>;

    WorksTuple co_works_tuple_;

  public:
    wait_any_task(CoTasks&... co_tasks) noexcept :
      co_works_tuple_{ co_tasks.get_work()... }
    {
    }

    wait_any_task(const wait_any_task&) = delete;
    wait_any_task& operator=(const wait_any_task&) = delete;

  private:
    class [[nodiscard]] awaiter
    {
      using SharedData = impl::wait_any_awaiter_shared_data<T>;
      using ChainDataTuple =
        std::tuple<
          impl::wait_any_awaiter_chain_data<SharedData, co_task_work_t<CoTasks>>...>;

      SharedData shared_data_;
      ChainDataTuple chain_data_;

    public:
      awaiter(context& parent_ctx, co_task_work_t<CoTasks>&... co_works) noexcept :
        shared_data_{ parent_ctx },
        chain_data_{ impl::wait_any_awaiter_chain_data_tuple_builder{shared_data_, co_works}... }
      {
      }

      awaiter(const awaiter&) = delete;
      awaiter& operator=(const awaiter&) = delete;

      [[nodiscard]] constexpr bool await_ready() const noexcept
      {
        return false;
      }

    //   bool await_suspend(std::coroutine_handle<> handle) noexcept
    //   {
    //     parent_handle_ = handle;
  
    //     for(auto& child: children_chain_data_)
    //     {
    //       child.resume();
    //     }
  
    //     --pending_count_;
    //     if (0 == pending_count_)
    //     {
    //       return false;
    //     }
    //     stop_cb_.enable(parent_ctx_.get_stop_token(), &awaiter::cancel, this);
    //     return true;
    //   }

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

    //   void cancel() noexcept
    //   {
    //     stop_cb_.disable();
    //     wait_stop_source_.request_stop();
    //     parent_ctx_.push_ready_node(node_, std::coroutine_handle<>());
    //   }
    };

    struct [[nodiscard]] work
    {
      WorksTuple co_works_tuple_;

      work(WorksTuple&& co_works_tuple) noexcept:
        co_works_tuple_{ std::move(co_works_tuple) }
      {
      }

      work(const work&) = delete;
      work& operator=(const work&) = delete;
      work(work&&) noexcept = default;
      work& operator=(work&&) noexcept = default;

      [[nodiscard]] awaiter get_awaiter(context& ctx) noexcept
      {
        return get_awaiter_impl(WorksTupleSeq{}, ctx);
      }

    private:
      template<std::size_t... I>
      [[nodiscard]] awaiter get_awaiter_impl(std::index_sequence<I...>, context& ctx) noexcept
      {
        return awaiter{ ctx, std::get<I>(co_works_tuple_)... };
      }
    };

  public:
    [[nodiscard]] work get_work() noexcept
    {
      return { std::move(co_works_tuple_) };
    }
  };

  template<is_co_task... CoTasks>
  [[nodiscard]] wait_any_task<CoTasks...>
    async_wait_any(CoTasks... co_tasks)
  {
    return wait_any_task<CoTasks...>{ co_tasks... };
  }
}