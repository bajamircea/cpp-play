#pragma once

#include "event_loop.h"
#include "event_loop_context.h"
#include "chain_context.h"
#include "context.h"

#include <thread>

namespace coro_st
{
  // TODO: continue and write test
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