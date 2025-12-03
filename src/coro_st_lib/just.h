#pragma once

#include "context.h"

#include <coroutine>
#include <type_traits>
#include <utility>

namespace coro_st
{
  template<typename T>
  class [[nodiscard]] just_task
  {
    class [[nodiscard]] awaiter
    {
      context& ctx_;
      T value_;

    public:
      awaiter(context& ctx, T value) noexcept :
        ctx_{ ctx },
        value_{ std::move(value) }
      {
      }

      awaiter(const awaiter&) = delete;
      awaiter& operator=(const awaiter&) = delete;

      [[nodiscard]] constexpr bool await_ready() const noexcept
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

      T await_resume() noexcept
      {
        return std::move(value_);
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        return {};
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
      T value_;

      explicit work(T value) noexcept :
        value_{ std::move(value) }
      {
      }

      work(const work&) = delete;
      work& operator=(const work&) = delete;
      work(work&&) noexcept = default;
      work& operator=(work&&) noexcept = default;

      [[nodiscard]] awaiter get_awaiter(context& ctx) noexcept
      {
        return {ctx, std::move(value_)};
      }
    };

  private:
    work work_;

  public:
    explicit just_task(T value) noexcept :
      work_{ std::move(value) }
    {
    }

    just_task(const just_task&) = delete;
    just_task& operator=(const just_task&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return std::move(work_);
    }
  };

  template<typename T>
    requires(std::is_nothrow_move_constructible_v<T>)
  [[nodiscard]] just_task<T> async_just(T value) noexcept
  {
    return just_task{ std::move(value) };
  }
}
