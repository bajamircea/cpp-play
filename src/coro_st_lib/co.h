#pragma once

#include "context.h"
#include "coro_type_traits.h"
#include "unique_coroutine_handle.h"
#include "promise_base.h"

#include <cassert>
#include <coroutine>
#include <utility>

namespace coro_st
{
  template<typename T>
  class [[nodiscard]] co
  {
  public:
    class promise_type : public promise_base<T>
    {
      friend co;

      context* pctx_{ nullptr };
      std::coroutine_handle<> parent_coro_;

    public:
      promise_type() noexcept = default;

      promise_type(const promise_type&) = delete;
      promise_type& operator=(const promise_type&) = delete;

      co get_return_object() noexcept
      {
        return { std::coroutine_handle<promise_type>::from_promise(*this) };
      }

      std::suspend_always initial_suspend() noexcept
      {
        return {};
      }

      struct final_awaiter
      {
        context& ctx_;

        [[nodiscard]] constexpr bool await_ready() const noexcept
        {
          return false;
        }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> child_coro) noexcept
        {
          auto parent_coro = child_coro.promise().parent_coro_;
          if (parent_coro)
          {
            return parent_coro;
          }

          // we're the first one in a chain
          if (ctx_.get_stop_token().stop_requested())
          {
            ctx_.schedule_cancellation_callback();
          }
          else
          {
            callback continuation_cb = ctx_.get_continuation_callback();
            continuation_cb.invoke();
          }

          return std::noop_coroutine();
        }

        [[noreturn]] constexpr void await_resume() const noexcept
        {
          std::unreachable();
          //std::terminate();
        }
      };

      final_awaiter final_suspend() noexcept
      {
        assert(pctx_ != nullptr);
        return {*pctx_};
      }

      template<coro_st::is_co_task CoTask>
      auto await_transform(CoTask co_task)
      {
        assert(pctx_ != nullptr);
        return co_task.get_work().get_awaiter(*pctx_);
      }
    };

  private:
    class [[nodiscard]] awaiter
    {
      unique_coroutine_handle<promise_type> unique_child_coro_;

    public:
      awaiter(context& ctx, unique_coroutine_handle<promise_type>&& unique_child_coro) noexcept :
        unique_child_coro_{ std::move(unique_child_coro) }
      {
        unique_child_coro_.get().promise().pctx_ = &ctx;
      }

      awaiter(const awaiter&) = delete;
      awaiter& operator=(const awaiter&) = delete;

      [[nodiscard]] constexpr bool await_ready() const noexcept
      {
        return false;
      }

      std::coroutine_handle<promise_type> await_suspend(std::coroutine_handle<> parent_coro) noexcept
      {
        std::coroutine_handle<promise_type> child_coro = unique_child_coro_.get();
        assert(!child_coro.promise().parent_coro_);
        child_coro.promise().parent_coro_ = parent_coro;
        return child_coro;
      }

      T await_resume() const
      {
        return unique_child_coro_.get().promise().get_result();
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        return unique_child_coro_.get().promise().get_result_exception();
      }

      void start_as_chain_root() noexcept
      {
        unique_child_coro_.get().resume();
      }
    };

    class [[nodiscard]] work
    {
      unique_coroutine_handle<promise_type> unique_child_coro_;

    public:
      work(std::coroutine_handle<promise_type> child_coro) noexcept :
        unique_child_coro_{ child_coro }
      {
      }

      work(const work&) = delete;
      work& operator=(const work&) = delete;
      work(work&&) noexcept = default;
      work& operator=(work&&) noexcept = default;

      [[nodiscard]] awaiter get_awaiter(context& ctx) noexcept
      {
        return { ctx, std::move(unique_child_coro_) };
      }
    };

  private:
    work work_;

    co(std::coroutine_handle<promise_type> child_coro) noexcept :
      work_{ child_coro }
    {
    }

  public:
    co(const co&) = delete;
    co& operator=(const co&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return std::move(work_);
    }
  };
}