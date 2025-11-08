#pragma once

#include "chain_context.h"

#include <coroutine>

namespace coro_st
{
  class context
  {
    chain_context& chain_ctx_;

  public:
    context(chain_context& chain_ctx) noexcept :
      chain_ctx_{ chain_ctx }
    {
    }

    context(context&, chain_context& new_chain_ctx) noexcept :
      chain_ctx_{ new_chain_ctx }
    {
    }

    context(const context&) = delete;
    context& operator=(const context&) = delete;

    stop_token get_stop_token() noexcept
    {
      return chain_ctx_.get_stop_token();
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