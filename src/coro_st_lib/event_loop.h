#pragma once

#include "ready_queue.h"
#include "timer_heap.h"

#include <cassert>
#include <chrono>
#include <optional>

namespace coro_st
{
  struct event_loop
  {
    ready_queue ready_queue_;
    timer_heap timers_heap_;

    event_loop() noexcept = default;

    event_loop(const event_loop&) = delete;
    event_loop& operator=(const event_loop&) = delete;

    std::optional<std::chrono::steady_clock::duration> do_current_pending_work() noexcept
    {
      coro_st::ready_queue local_ready = std::move(ready_queue_);
      while (!local_ready.empty())
      {
        auto* ready_node = local_ready.pop();

        callback cb = ready_node->cb;
        assert(cb.is_callable());
        cb.invoke();
      }
      if (timers_heap_.min_node() != nullptr)
      {
        auto now = std::chrono::steady_clock::now();
        do
        {
          auto* timer_node = timers_heap_.min_node();
          if (timer_node->deadline > now)
          {
            if (ready_queue_.empty())
            {
              return {timer_node->deadline - now};
            }
            break;
          }

          timers_heap_.pop_min();

          callback cb = timer_node->cb;
          assert(cb.is_callable());
          cb.invoke();
        } while(timers_heap_.min_node() != nullptr);
      }
      return std::nullopt;
    }
  };
}