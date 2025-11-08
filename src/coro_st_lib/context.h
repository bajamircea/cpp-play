#pragma once

#include "stop_util.h"
#include <coroutine>

namespace coro_st
{
  class context
  {
    stop_token token_;

  public:
    explicit context(stop_token token) noexcept :
      token_{ token }
    {
    }


    context(const context&) = delete;
    context& operator=(const context&) = delete;

    stop_token get_stop_token() noexcept
    {
      return token_;
    }

    void invoke_continuation() noexcept
    {
    }

    void schedule_continuation() noexcept
    {
    }

    void invoke_cancellation() noexcept
    {
    }

    void schedule_cancellation() noexcept
    {
    }

    void schedule_coroutine_resume(std::coroutine_handle<void>) noexcept
    {
    }
  };
}