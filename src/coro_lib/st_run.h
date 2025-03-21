#pragma once

#include "deferred_co.h"
#include "trampoline_co.h"
#include "st_ready_queue.h"
#include "st_timer_heap.h"
#include "st_context.h"
#include "st_type_traits.h"

#include <cassert>
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
        assert(ready_node->chain_ctx != nullptr);
        ready_node->chain_ctx->on_resume(ready_node->coroutine);
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
          assert(timer_node->chain_ctx != nullptr);
          timer_node->chain_ctx->on_resume(timer_node->coroutine);
        } while(timers_heap_.min_node() != nullptr);
      }
    }
  };

  template<is_context_callable_co CoFn>
  auto run(CoFn&& co_fn)
    -> context_callable_await_result_t<CoFn>
  {
    runner_impl runner;

    stop_source main_stop_source;

    runner_context runner_ctx(runner.ready_queue_, runner.timers_heap_);
    chain_context main_chain_ctx{
      main_stop_source.get_token(),
      [](void* ,std::coroutine_handle<> coroutine) noexcept {
        coroutine.resume();
      },
      nullptr
    };
    context main_ctx(runner_ctx, main_chain_ctx);

    using TrampolineType = coro::trampoline_co<context_callable_await_result_t<CoFn>>;
    auto trampoline = [](context& ctx, CoFn& co_fn)
     -> TrampolineType {
      co_return co_await co_fn(ctx);
    };

    bool done{ false };
    OnTrampolineDoneFnPtr on_done = +[](void* x) noexcept {
      bool* p_done = reinterpret_cast<bool*>(x);
      *p_done = true;
    };
    auto main_co = trampoline(main_ctx, co_fn);
    main_co.set_on_done_fn(on_done, &done);
    main_co.resume();

    while (!done)
    {
      runner.do_work();
    }

    return main_co.get_result();
  }
}