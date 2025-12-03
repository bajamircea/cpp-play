#pragma once

#include "context.h"

#include <coroutine>
#include <exception>
#include <type_traits>
#include <utility>

namespace coro_st
{
  class [[nodiscard]] just_exception_task
  {
    class [[nodiscard]] awaiter
    {
      context& ctx_;
      std::exception_ptr ex_;

    public:
      awaiter(context& ctx, std::exception_ptr ex) noexcept :
        ctx_{ ctx },
        ex_{ std::move(ex) }
      {
      }

      awaiter(const awaiter&) = delete;
      awaiter& operator=(const awaiter&) = delete;

      [[nodiscard]] bool await_ready() const noexcept
      {
        if (ctx_.get_stop_token().stop_requested())
        {
          return false;
        }
        return true;
      }

      void await_suspend(std::coroutine_handle<>) noexcept
      {
        ctx_.schedule_stopped();
      }

      void await_resume() const
      {
        std::rethrow_exception(ex_);
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        return ex_;
      }

      void start() noexcept
      {
        if (ctx_.get_stop_token().stop_requested())
        {
          ctx_.invoke_stopped();
          return;
        }
        ctx_.invoke_result_ready();
      }
    };

    struct [[nodiscard]] work
    {
      std::exception_ptr ex_;

      explicit work(std::exception_ptr ex) noexcept :
        ex_{ std::move(ex) }
      {
      }

      work(const work&) = delete;
      work& operator=(const work&) = delete;
      work(work&&) noexcept = default;
      work& operator=(work&&) noexcept = default;

      [[nodiscard]] awaiter get_awaiter(context& ctx) noexcept
      {
        return {ctx, std::move(ex_)};
      }
    };

  private:
    work work_;

  public:
    explicit just_exception_task(std::exception_ptr ex) noexcept :
      work_{ std::move(ex) }
    {
      static_assert(std::is_nothrow_move_constructible_v<std::exception_ptr>);
    }

    just_exception_task(const just_exception_task&) = delete;
    just_exception_task& operator=(const just_exception_task&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return std::move(work_);
    }
  };

  template<typename T>
  [[nodiscard]] just_exception_task async_just_exception(T ex) noexcept
  {
    try
    {
      throw ex;
    }
    catch(...)
    {
      return just_exception_task{ std::current_exception() };
    }
  }
}
