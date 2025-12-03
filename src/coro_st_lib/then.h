#pragma once

#include "context.h"
#include "coro_type_traits.h"
#include "stop_util.h"
#include "value_type_traits.h"

#include <cassert>
#include <coroutine>
#include <type_traits>
#include <variant>

namespace coro_st
{
  template<is_co_task CoTask, typename Fn>
  struct [[nodiscard]] then_task
  {
    using CoWork = co_task_work_t<CoTask>;
    using CoAwaiter = co_task_awaiter_t<CoTask>;
    using TaskResultType = co_task_result_t<CoTask>;
    using FnResultType =
      std::conditional_t<
          std::is_same_v<void, TaskResultType>,
          std::invoke_result<Fn>,
          std::invoke_result<Fn, TaskResultType>>::type;
    using T = coro_st::value_type_traits::value_type_t<FnResultType>;

    class [[nodiscard]] awaiter
    {
      static constexpr size_t g_none_result_type = 0;
      static constexpr size_t g_value_result_type = 1;
      static constexpr size_t g_error_result_type = 2;
      static constexpr size_t g_stopped_result_type = 3;

      context& parent_ctx_;
      std::coroutine_handle<> parent_handle_;
      bool pending_start_{ false };
      std::variant<std::monostate, T, std::exception_ptr, std::monostate> result_;

      Fn fn_;

      context task_ctx_;
      CoAwaiter co_awaiter_;

    public:
      awaiter(
        context& parent_ctx,
        CoWork& co_work,
        Fn fn
      ) :
        parent_ctx_{ parent_ctx },
        parent_handle_{},
        pending_start_{ false },
        result_{},
        fn_{ std::move(fn) },
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

        if (g_none_result_type == result_.index())
        {
          return true;
        }

        if (g_stopped_result_type == result_.index())
        {
          parent_ctx_.schedule_stopped();
          return true;
        }

        return false;
      }

      T await_resume()
      {
        if (g_error_result_type == result_.index())
        {
          std::rethrow_exception(std::get<g_error_result_type>(result_));
        }

        if constexpr (std::is_same_v<void, T>)
        {
          return;
        }
        else
        {
          return std::move(std::get<g_value_result_type>(result_));
        }
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        if (g_error_result_type == result_.index())
        {
          return std::get<g_error_result_type>(result_);
        }
        return {};
      }

      void start() noexcept
      {
        pending_start_ = true;
        co_awaiter_.start();
        pending_start_ = false;

        if (g_none_result_type == result_.index())
        {
          return;
        }

        if (g_stopped_result_type == result_.index())
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

        if (g_stopped_result_type == result_.index())
        {
          parent_ctx_.schedule_stopped();
          return;
        }

        if (parent_handle_)
        {
          parent_ctx_.schedule_coroutine_resume(parent_handle_);
          return;
        }

        parent_ctx_.schedule_result_ready();
      }

      void on_task_result_ready() noexcept
      {
        if (g_none_result_type == result_.index())
        {
          try
          {
            if constexpr (std::is_same_v<void, TaskResultType>)
            {
              if constexpr (std::is_same_v<void, FnResultType>)
              {
                fn_();
                result_.template emplace<g_value_result_type>(coro_st::void_result{});
              }
              else
              {
                result_.template emplace<g_value_result_type>(fn_());
              }
            }
            else
            {
              if constexpr (std::is_same_v<coro_st::void_result, T>)
              {
                fn_(co_awaiter_.await_resume());
                result_.template emplace<g_value_result_type>(coro_st::void_result{});
              }
              else
              {
                result_.template emplace<g_value_result_type>(fn_(co_awaiter_.await_resume()));
              }
            }
          }
          catch(...)
          {
            result_.template emplace<g_error_result_type>(std::current_exception());
          }
        }
        on_shared_continue();
      }

      void on_task_stopped() noexcept
      {
        result_.template emplace<g_stopped_result_type>();
        on_shared_continue();
      }
    };

    struct [[nodiscard]] work
    {
      CoWork co_work_;
      Fn fn_;

      work(CoTask& co_task, Fn fn) noexcept:
        co_work_{ co_task.get_work() },
        fn_{ std::move(fn) }
      {
      }

      work(const work&) = delete;
      work& operator=(const work&) = delete;
      work(work&&) noexcept = default;
      work& operator=(work&&) noexcept = default;

      [[nodiscard]] awaiter get_awaiter(context& ctx)
      {
        return {ctx, co_work_, std::move(fn_)};
      }
    };

  private:
    work work_;

  public:
    then_task(CoTask& co_task, Fn fn) noexcept :
      work_{ co_task, std::move(fn) }
    {
    }

    then_task(const then_task&) = delete;
    then_task& operator=(const then_task&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return std::move(work_);
    }
  };

  template<is_co_task CoTask, typename Fn>
  [[nodiscard]] then_task<CoTask, Fn>
    async_then(CoTask co_task, Fn&& fn)
  {
    using result_t = co_task_result_t<CoTask>;
    if constexpr (std::is_same_v<void, result_t>)
    {
      static_assert(std::is_invocable_v<Fn>);
    }
    else
    {
      static_assert(std::is_invocable_v<Fn, co_task_result_t<CoTask>>);
    }
    return then_task<CoTask, Fn>{ co_task, std::forward<Fn>(fn) };
  }
}