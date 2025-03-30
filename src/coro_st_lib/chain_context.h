#pragma once

#include "callback.h"
#include "stop_util.h"
#include "ready_queue.h"

#include <cassert>

namespace coro_st
{
  class chain_context
  {
    stop_token token_;
    callback continuation_cb_{};
    callback cancellation_cb_{};
    ready_node node_{};

  public:
    chain_context(
      stop_token token,
      callback continuation_cb,
      callback cancellation_cb
    ) noexcept :
      token_{ token },
      continuation_cb_{ continuation_cb },
      cancellation_cb_{ cancellation_cb }
    {
    }

    chain_context(const chain_context&) = delete;
    chain_context& operator=(const chain_context&) = delete;

    stop_token get_stop_token() noexcept
    {
      return token_;
    }

    callback get_continuation_callback() noexcept
    {
      return continuation_cb_;
    }

    callback get_cancellation_callback() noexcept
    {
      return cancellation_cb_;
    }

    ready_node& get_chain_node() noexcept
    {
      return node_;
    }
  };
}