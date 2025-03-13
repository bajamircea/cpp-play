#pragma once

#include "co.h"
#include "deferred_co.h"
#include "trampoline_co.h"
#include "st_context.h"
#include "st_type_traits.h"

#include <type_traits>

namespace coro::st
{
  // struct runner_impl
  // {
  //   ready_queue ready_queue_;
  //   timer_heap timers_heap_;

  //   runner_impl() noexcept = default;

  //   runner_impl(const runner_impl &) = delete;
  //   runner_impl& operator=(const runner_impl &) = delete;

  //   void do_work() {
  //     cpp_util::intrusive_queue local_ready = std::move(ready_queue_);
  //     while (!local_ready.empty())
  //     {
  //       auto* ready_node = local_ready.pop();
  //       assert(ready_node->chain_ctx != nullptr);
  //       ready_node->chain_ctx->on_resume(ready_node->coroutine);
  //     }
  //     if (timers_heap_.min_node() != nullptr)
  //     {
  //       auto now = std::chrono::steady_clock::now();
  //       do
  //       {
  //         auto* timer_node = timers_heap_.min_node();
  //         if (timer_node->deadline > now)
  //         {
  //           if (ready_queue_.empty())
  //           {
  //             std::this_thread::sleep_for(timer_node->deadline - now);
  //           }
  //           break;
  //         }
  //         timers_heap_.pop_min();
  //         assert(timer_node->chain_ctx != nullptr);
  //         timer_node->chain_ctx->on_resume(timer_node->coroutine);
  //       } while(timers_heap_.min_node() != nullptr);
  //     }
  //   }
  // };

  template<typename T>
  struct wait_any_result
  {
    size_t index{};
    T result{};
  };

  template<is_deferred_context_co... DeferredCoFn>
  auto async_wait_any(coro::st::context & ctx, DeferredCoFn&&... co_fns)
    -> coro::co<
        wait_any_result<
          std::common_type_t<
            deferred_context_co_return_type<DeferredCoFn>...>>>
  {
    co_return wait_any_result{.index=0, .result=0};
  }

  // template<is_deferred_context_co DeferredCoFn>
  // auto run(DeferredCoFn&& co_fn)
  //   -> deferred_context_co_return_type<DeferredCoFn>
  // {
  //   runner_impl runner;

  //   stop_source main_stop_source;

  //   runner_context runner_ctx(runner.ready_queue_, runner.timers_heap_);
  //   custom_chain_context chain_ctx{
  //     main_stop_source.get_token(),
  //     [](std::coroutine_handle<> coroutine) noexcept {
  //       coroutine.resume();
  //     }
  //   };
  //   context main_ctx(runner_ctx, chain_ctx);

  //   using TrampolineType = coro::trampoline_co<deferred_context_co_return_type<DeferredCoFn>>;
  //   auto trampoline = [](context& ctx, DeferredCoFn& co_fn)
  //    -> TrampolineType {
  //     co_return co_await co_fn(ctx);
  //   };

  //   bool done{ false };
  //   OnTrampolineDoneFnPtr on_done = +[](void* x) noexcept {
  //     bool* p_done = reinterpret_cast<bool*>(x);
  //     *p_done = true;
  //   };
  //   auto main_co = trampoline(main_ctx, co_fn);
  //   main_co.set_fn(on_done, &done);
  //   main_co.resume();

  //   while (!done)
  //   {
  //     runner.do_work();
  //   }

  //   return main_co.get_result();
  // }
}