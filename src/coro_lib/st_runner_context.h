#pragma once

#include "st_chain_context.h"
#include "st_ready_queue.h"
#include "st_timer_heap.h"

namespace coro::st
{
  class runner_context
  {
    ready_queue& ready_queue_;
    timer_heap& timers_heap_;
  public:
    runner_context(ready_queue& ready_queue, timer_heap& timers_heap) noexcept :
      ready_queue_{ ready_queue }, timers_heap_{ timers_heap }
    {
    }

    runner_context(const runner_context&) = delete;
    runner_context& operator=(const runner_context&) = delete;

    void push_ready_node(ready_node& node, chain_context& chain_ctx, std::coroutine_handle<> handle) noexcept
    {
      node.coroutine = handle;
      node.chain_ctx = &chain_ctx;
      ready_queue_.push(&node);
    }

    void insert_timer_node(timer_node& node, chain_context& chain_ctx, std::coroutine_handle<> handle) noexcept
    {
      node.coroutine = handle;
      node.chain_ctx = &chain_ctx;
      timers_heap_.insert(&node);
    }

    void remove_timer_node(timer_node& node) noexcept
    {
      timers_heap_.remove(&node);
    }
  };
}