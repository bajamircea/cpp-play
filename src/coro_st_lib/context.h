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
  };
}