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
      }

      void on_cancel() noexcept
      {
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

  public://TODO
    class [[nodiscard]] awaiter
    {
    public: // TODO
      template<is_co_awaiter CoAwaiter>
      class chain_data
      {
        awaiter& awaiter_;
        size_t index_;
        chain_context chain_ctx_;
        context ctx_;
        CoAwaiter co_awaiter_;
      public:
        template<is_co_work CoWork>
        chain_data(awaiter& awaiter, CoWork& co_work) :
          awaiter_{ awaiter },
          index_{ awaiter_.pending_count_++ },
          chain_ctx_{
            awaiter_.wait_stop_source_.get_token(),
            make_member_callback<&chain_data::on_continue>(this),
            make_member_callback<&chain_data::on_cancel>(this),
            },
          ctx_{ awaiter_.parent_ctx_, chain_ctx_ },
          co_awaiter_{ co_work.get_awaiter(ctx_) }
        {
        }

        chain_data(const chain_data&) = delete;
        chain_data& operator=(const chain_data&) = delete;

        void on_continue() noexcept
        {
          if (!awaiter_.parent_ctx_.get_stop_token().stop_requested())
          {
            if (N == awaiter_.data_result_.index)
            {
              awaiter_.data_result_.index = index_;
              awaiter_.wait_stop_source_.request_stop();
              try
              {
                if constexpr (std::is_same_v<void, co_awaiter_result_t<CoAwaiter>>)
                {
                  //co_awaiter_.await_resume();
                }
                else
                {
                  //awaiter_.data_result_.value = co_awaiter_.await_resume();
                }
              }
              catch(...)
              {
                awaiter_.exception_result_ = std::current_exception();
              }
            }
            --awaiter_.pending_count_;
            if (0 != awaiter_.pending_count_)
            {
              return;
            }
            awaiter_.on_done();
          }
        }

        void on_cancel() noexcept
        {
          --awaiter_.pending_count_;
          if (0 != awaiter_.pending_count_)
          {
            return;
          }
          awaiter_.on_done();
        }
      };

      using ChildrenTuple = std::tuple<chain_data<co_task_awaiter_t<CoTasks>>...>;

      // awaiter members
      context& parent_ctx_;
      std::coroutine_handle<> parent_handle_;
      stop_source wait_stop_source_;
      std::exception_ptr exception_result_;
      ResultType data_result_;
    //   stop_op_callback<awaiter> stop_cb_;
      size_t pending_count_;
      //std::tuple<chain_data<co_task_awaiter_t<CoTasks>>...> children_;

    public:
      template<is_co_work... CoWorks>
      awaiter(context& parent_ctx, CoWorks&&... co_works) noexcept :
        parent_ctx_{ parent_ctx },
        parent_handle_{},
        wait_stop_source_{},
        exception_result_{},
        data_result_{.index=N},
        pending_count_{ 0 }//,
        //children_{{*this, std::forward<CoWorks>(co_works)}...}
      {
        static_assert(N == sizeof...(co_works));
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
        assert(data_result_.index != N);
        if (exception_result_)
        {
          std::rethrow_exception(exception_result_);
        }
        return std::move(data_result_);
      }

    //   void cancel() noexcept
    //   {
    //     stop_cb_.disable();
    //     wait_stop_source_.request_stop();
    //     parent_ctx_.push_ready_node(node_, std::coroutine_handle<>());
    //   }

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
        return awaiter{ ctx, std::move(std::get<I>(co_works_tuple_))... };
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