#pragma once

#include "../coro_st_lib/context.h"
#include "../coro_st_lib/coro_type_traits.h"
#include "../coro_st_lib/stop_util.h"
#include "../coro_st_lib/value_type_traits.h"

#include <cassert>
#include <coroutine>
#include <exception>
#include <memory>
#include <variant>

namespace coro_st_test
{
  template<coro_st::is_co_task CoTask>
  class [[nodiscard]] deleter_task
  {
    static constexpr size_t g_none_result_type = 0;
    static constexpr size_t g_value_result_type = 1;
    static constexpr size_t g_error_result_type = 2;
    static constexpr size_t g_stopped_result_type = 3;

    using CoWork = coro_st::co_task_work_t<CoTask>;
    using CoAwaiter = coro_st::co_task_awaiter_t<CoTask>;
    using T = coro_st::co_task_result_t<CoTask>;
    using ResultType = coro_st::value_type_traits::value_type_t<T>;

    class [[nodiscard]] awaiter
    {
      struct AwaiterWrap
      {
        CoAwaiter co_awaiter_;

        AwaiterWrap(
          coro_st::context& task_ctx_,
          CoWork& co_work
        ) :
          co_awaiter_{ co_work.get_awaiter(task_ctx_) }
        {
        }
      };

      coro_st::context& parent_ctx_;
      std::coroutine_handle<> parent_handle_;
      bool pending_start_{ false };
      // see g_..._result_type above
      std::variant<std::monostate, ResultType, std::exception_ptr, std::monostate> result_;

      coro_st::context task_ctx_;
      std::unique_ptr<AwaiterWrap> ptr_;

    public:
      awaiter(
        coro_st::context& parent_ctx,
        CoWork& co_work
      ) :
        parent_ctx_{ parent_ctx },
        parent_handle_{},
        pending_start_{ false },
        result_{},
        task_ctx_{
          parent_ctx_,
          parent_ctx_.get_stop_token(),
          coro_st::make_member_completion<
            &awaiter::on_task_result_ready,
            &awaiter::on_task_stopped
            >(this)
          },
        ptr_{ std::make_unique<AwaiterWrap>(task_ctx_, co_work) }
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
        ptr_->co_awaiter_.start();
        pending_start_ = false;

        if (deleter_task::g_none_result_type == result_.index())
        {
          return true;
        }

        if (deleter_task::g_stopped_result_type == result_.index())
        {
          parent_ctx_.invoke_stopped();
          return true;
        }

        return false;
      }

      ResultType await_resume()
      {
        switch(result_.index())
          {
            case deleter_task::g_value_result_type:
              return std::move(std::get<deleter_task::g_value_result_type>(result_));
            case deleter_task::g_error_result_type:
              std::rethrow_exception(std::get<deleter_task::g_error_result_type>(result_));
            default:
              std::terminate();
          }
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        if (deleter_task::g_error_result_type != result_.index())
        {
          return {};
        }
        return std::get<deleter_task::g_error_result_type>(result_);
      }

      void start() noexcept
      {
        pending_start_ = true;
        ptr_->co_awaiter_.start();
        pending_start_ = false;

        if (deleter_task::g_none_result_type == result_.index())
        {
          return;
        }

        if (deleter_task::g_stopped_result_type == result_.index())
        {
          parent_ctx_.invoke_stopped();
          return;
        }

        parent_ctx_.invoke_result_ready();
      }

    private:
      void on_shared_continue() noexcept
      {
        ptr_.reset();

        if (pending_start_)
        {
          return;
        }

        if (deleter_task::g_stopped_result_type == result_.index())
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
        assert(deleter_task::g_none_result_type == result_.index());

        std::exception_ptr e = ptr_->co_awaiter_.get_result_exception();
        if (e)
        {
          result_.template emplace<deleter_task::g_error_result_type>(e);
        }
        else
        {
          try
          {
            if constexpr (std::is_same_v<void, T>)
            {
              ptr_->co_awaiter_.await_resume();
              result_.template emplace<deleter_task::g_value_result_type>();
            }
            else
            {
              result_.template emplace<deleter_task::g_value_result_type>(ptr_->co_awaiter_.await_resume());
            }
          }
          catch(...)
          {
            result_.template emplace<deleter_task::g_error_result_type>(std::current_exception());
          }
        }

        on_shared_continue();
      }

      void on_task_stopped() noexcept
      {
        assert(deleter_task::g_none_result_type == result_.index());

        result_.template emplace<deleter_task::g_stopped_result_type>();
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

      [[nodiscard]] awaiter get_awaiter(coro_st::context& ctx)
      {
        return {ctx, co_work_};
      }
    };

  private:
    work work_;

  public:
    deleter_task(CoTask& co_task) noexcept :
      work_{ co_task }
    {
    }

    deleter_task(const deleter_task&) = delete;
    deleter_task& operator=(const deleter_task&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return std::move(work_);
    }
  };

  template<coro_st::is_co_task CoTask>
  [[nodiscard]] deleter_task<CoTask>
    async_deleter(CoTask co_task)
  {
    return deleter_task<CoTask>{ co_task };
  }
}