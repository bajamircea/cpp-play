#pragma once

#include <coroutine>

namespace coro_test
{
  struct dummy_awaiter_suspend_void
  {
    dummy_awaiter_suspend_void(const dummy_awaiter_suspend_void&) = delete;
    dummy_awaiter_suspend_void& operator=(const dummy_awaiter_suspend_void&) = delete;

    [[nodiscard]] constexpr bool await_ready() const noexcept
    {
      return false;
    }

    void await_suspend(std::coroutine_handle<>) noexcept
    {
    }

    void await_resume() const
    {
    }
  };
}