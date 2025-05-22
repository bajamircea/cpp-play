#pragma once

#include "event_loop.h"
#include "event_loop_context.h"
#include "chain_context.h"
#include "context.h"
#include "coro_type_traits.h"

#include <thread>

namespace coro_st
{
  template<is_co_task CoTask>
  auto run(CoTask co_task)
    -> co_task_result_t<CoTask>
  {
    stop_source main_stop_source;
    bool done { false };

    event_loop el;

    event_loop_context el_ctx{ el.ready_queue_, el.timers_heap_ };
    chain_context chain_ctx{
      main_stop_source.get_token(),
      callback{ make_function_callback<+[](bool& x) noexcept {
        x = true;
      }>(done) },
      callback{ nullptr, +[](void*) noexcept {
        std::terminate();
      }}
    };
    context ctx{ el_ctx, chain_ctx };

    auto co_work = co_task.get_work();

    auto co_awaiter = co_work.get_awaiter(ctx);

    co_awaiter.start_as_chain_root();

    while (!done)
    {
      auto sleep_time = el.do_current_pending_work();
      if (sleep_time.has_value())
      {
        std::this_thread::sleep_for(*sleep_time);
      }
    }

    return co_awaiter.await_resume();
  }
}