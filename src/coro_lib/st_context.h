#pragma once

#include "st_stop.h"
#include "st_ready_queue.h"
#include "st_timer_heap.h"

namespace coro::st
{
  class context
  {
    coro::st::stop_token token_;
    ready_queue& ready_queue_;
    timer_heap& timers_heap_;

  public:
    context(coro::st::stop_token token, ready_queue& ready_queue, timer_heap& timers_heap) noexcept :
      token_{ token }, ready_queue_{ ready_queue }, timers_heap_{ timers_heap }
    {
    }

    context(coro::st::stop_token token, context& other) noexcept :
    token_{ token }, ready_queue_{ other.ready_queue_ }, timers_heap_{ other.timers_heap_ }
    {
    }

    context(const context &) = delete;
    context & operator=(const context &) = delete;

    void push_ready_node(ready_node& node, std::coroutine_handle<> handle) noexcept
    {
      node.coroutine = handle;
      ready_queue_.push(&node);
    }

    void insert_timer_node(timer_node& node, std::coroutine_handle<> handle) noexcept
    {
      node.coroutine = handle;
      timers_heap_.insert(&node);
    }
  };
}