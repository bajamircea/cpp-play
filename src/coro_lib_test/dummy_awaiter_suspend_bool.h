#pragma once

#include <coroutine>

namespace coro_test
{
  struct dummy_awaiter_suspend_bool
  {
    dummy_awaiter_suspend_bool(const dummy_awaiter_suspend_bool&) = delete;
    dummy_awaiter_suspend_bool& operator=(const dummy_awaiter_suspend_bool&) = delete;

    [[nodiscard]] constexpr bool await_ready() const noexcept
    {
      return false;
    }

    bool await_suspend(std::coroutine_handle<>) noexcept
    {
      return true;
    }

    void await_resume() const
    {
    }
  };
}