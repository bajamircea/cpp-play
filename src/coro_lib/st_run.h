#pragma once

#include "co.h"
#include "deferred_co.h"
#include "trampoline_co.h"
#include "st_ready_queue.h"
#include "st_timer_heap.h"
#include "st_context.h"

#include <chrono>
#include <thread>

namespace coro::st
{
  struct runner_impl
  {
    ready_queue ready_queue_;
    timer_heap timers_heap_;

    runner_impl() noexcept = default;

    runner_impl(const runner_impl &) = delete;
    runner_impl& operator=(const runner_impl &) = delete;

    void do_work() {
      cpp_util::intrusive_queue local_ready = std::move(ready_queue_);
      while (!local_ready.empty())
      {
        auto* ready_node = local_ready.pop();
        ready_node->coroutine.resume();
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
              std::this_thread::sleep_for(timer_node->deadline - now);
            }
            break;
          }
          timers_heap_.pop_min();
          timer_node->coroutine.resume();
        } while(timers_heap_.min_node() != nullptr);
      }
    }
  };

  template<typename DeferredCoFn>
  auto run(DeferredCoFn&& co_fn) -> DeferredCoFn::co_return_type
  {
    runner_impl runner;

    stop_source main_stop_source;
    context main_ctx(main_stop_source.get_token(), runner.ready_queue_, runner.timers_heap_);

    auto trampoline = [](context& ctx, DeferredCoFn& co_fn)
     -> coro::trampoline_co<typename DeferredCoFn::co_return_type> {
      co_return co_await co_fn(ctx);
    };

    auto main_co = trampoline(main_ctx, co_fn);
    main_co.resume();

    while (!main_co.done())
    {
      runner.do_work();
    }

    return main_co.get_result();
  }
}