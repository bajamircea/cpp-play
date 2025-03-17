#pragma once

#include "st_chain_context.h"
#include "st_runner_context.h"

namespace coro::st
{
  class context
  {
    runner_context& runner_ctx_;
    chain_context& chain_ctx_;

  public:
    context(runner_context& runner_ctx, chain_context& chain_ctx) noexcept :
      runner_ctx_{ runner_ctx }, chain_ctx_{ chain_ctx }
    {
    }

    context(context& parent_context, chain_context& chain_ctx) noexcept :
      runner_ctx_{ parent_context.runner_ctx_ }, chain_ctx_{ chain_ctx }
    {
    }

    context(const context&) = delete;
    context& operator=(const context&) = delete;

    void push_ready_node(ready_node& node, std::coroutine_handle<> handle) noexcept
    {
      runner_ctx_.push_ready_node(node, chain_ctx_, handle);
    }

    void insert_timer_node(timer_node& node, std::coroutine_handle<> handle) noexcept
    {
      runner_ctx_.insert_timer_node(node, chain_ctx_, handle);
    }

    void remove_timer_node(timer_node& node) noexcept
    {
      runner_ctx_.remove_timer_node(node);
    }

    stop_token get_stop_token() noexcept
    {
      return chain_ctx_.get_stop_token();
    }
  };
}