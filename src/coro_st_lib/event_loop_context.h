#pragma once

#include "ready_queue.h"
#include "timer_heap.h"

#include <cassert>

namespace coro_st
{
  class event_loop_context
  {
    ready_queue& ready_queue_;
    timer_heap& timer_heap_;
  public:
    event_loop_context(ready_queue& ready_queue, timer_heap& timer_heap) noexcept :
      ready_queue_{ ready_queue }, timer_heap_{ timer_heap }
    {
    }

    event_loop_context(const event_loop_context&) = delete;
    event_loop_context& operator=(const event_loop_context&) = delete;

    void push_ready_node(ready_node& node) noexcept
    {
      assert(node.cb.is_callable());
      ready_queue_.push(&node);
    }

    void insert_timer_node(timer_node& node) noexcept
    {
      assert(node.cb.is_callable());
      timer_heap_.insert(&node);
    }

    void remove_timer_node(timer_node& node) noexcept
    {
      timer_heap_.remove(&node);
    }
  };
}