#pragma once

#include "event_loop_context.h"
#include "chain_context.h"

#include <coroutine>

namespace coro_st
{
  class context
  {
    event_loop_context& event_loop_ctx_;
    chain_context& chain_ctx_;

  public:
    context(event_loop_context& event_loop_ctx, chain_context& chain_ctx) noexcept :
      event_loop_ctx_{ event_loop_ctx }, chain_ctx_{ chain_ctx }
    {
    }

    context(context& parent_context, chain_context& new_chain_ctx) noexcept :
      event_loop_ctx_{ parent_context.event_loop_ctx_ }, chain_ctx_{ new_chain_ctx }
    {
    }

    context(const context&) = delete;
    context& operator=(const context&) = delete;

    void push_ready_node(ready_node& node) noexcept
    {
      return event_loop_ctx_.push_ready_node(node);
    }

    void insert_timer_node(timer_node& node) noexcept
    {
      return event_loop_ctx_.insert_timer_node(node);
    }

    void remove_timer_node(timer_node& node) noexcept
    {
      return event_loop_ctx_.remove_timer_node(node);
    }

    stop_token get_stop_token() noexcept
    {
      return chain_ctx_.get_stop_token();
    }

    callback get_continuation_callback() noexcept
    {
      return chain_ctx_.get_continuation_callback();
    }

    void schedule_continuation_callback() noexcept
    {
      ready_node& node = chain_ctx_.get_chain_node();
      node.cb = chain_ctx_.get_continuation_callback();
      event_loop_ctx_.push_ready_node(node);
    }

    void schedule_coroutine_resume(std::coroutine_handle<void> handle) noexcept
    {
      ready_node& node = chain_ctx_.get_chain_node();
      node.cb = make_resume_coroutine_callback(handle);
      event_loop_ctx_.push_ready_node(node);
    }

    void schedule_cancellation_callback() noexcept
    {
      ready_node& node = chain_ctx_.get_chain_node();
      node.cb = chain_ctx_.get_cancellation_callback();
      event_loop_ctx_.push_ready_node(node);
    }

    ready_node& get_chain_node() noexcept
    {
      return chain_ctx_.get_chain_node();
    }
  };
}