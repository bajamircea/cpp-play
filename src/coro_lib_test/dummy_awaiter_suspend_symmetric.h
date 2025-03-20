#pragma once

#include <coroutine>

namespace coro_test
{
  struct dummy_awaiter_suspend_symmetric
  {
    dummy_awaiter_suspend_symmetric(const dummy_awaiter_suspend_symmetric&) = delete;
    dummy_awaiter_suspend_symmetric& operator=(const dummy_awaiter_suspend_symmetric&) = delete;

    [[nodiscard]] constexpr bool await_ready() const noexcept
    {
      return false;
    }

    auto await_suspend(std::coroutine_handle<>) noexcept
    {
      return std::noop_coroutine();
    }

    void await_resume() const
    {
    }
  };
}