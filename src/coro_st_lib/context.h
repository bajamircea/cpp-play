#pragma once

#include "event_loop_context.h"
#include "completion.h"
#include "stop_util.h"

#include <coroutine>

namespace coro_st
{
  class context
  {
    event_loop_context& event_loop_ctx_;
    stop_token token_;
    completion completion_;
    ready_node node_;

  public:
    context(
      event_loop_context& event_loop_ctx,
      stop_token token,
      completion completion
    ) noexcept :
      event_loop_ctx_{ event_loop_ctx },
      token_{ token },
      completion_{ completion },
      node_{}
    {
    }

    context(
      context& parent_context,
      stop_token token,
      completion completion
    ) noexcept :
      event_loop_ctx_{ parent_context.event_loop_ctx_ },
      token_{ token },
      completion_{ completion },
      node_{}
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
      return token_;
    }

    void invoke_result_ready() noexcept
    {
      callback cb = completion_.get_result_ready_callback();
      cb.invoke();
    }

    void schedule_result_ready() noexcept
    {
      node_.cb = completion_.get_result_ready_callback();
      event_loop_ctx_.push_ready_node(node_);
    }

    void invoke_stopped() noexcept
    {
      callback cb = completion_.get_stopped_callback();
      cb.invoke();
    }

    void schedule_stopped() noexcept
    {
      node_.cb = completion_.get_stopped_callback();
      event_loop_ctx_.push_ready_node(node_);
    }

    void schedule_coroutine_resume(std::coroutine_handle<void> handle) noexcept
    {
      node_.cb = make_resume_coroutine_callback(handle);
      event_loop_ctx_.push_ready_node(node_);
    }

    ready_node& get_chain_node() noexcept
    {
      return node_;
    }
  };
}