#pragma once

#include "event_loop.h"
#include "event_loop_context.h"
#include "chain_context.h"
#include "context.h"
#include "coro_type_traits.h"

#include <thread>

namespace coro_st
{
  template<is_co_awaitable CoAwaitable>
  auto run(CoAwaitable co_awaitable)
    -> co_awaitable_result_t<CoAwaitable>
  {
    stop_source main_stop_source;
    bool done { false };

    event_loop el;

    event_loop_context el_ctx{ el.ready_queue_, el.timers_heap_ };
    chain_context chain_ctx{
      main_stop_source.get_token(),
      callback{ &done, +[](void* x) noexcept {
        bool* p_done = reinterpret_cast<bool*>(x);
        *p_done = true;
      }},
      callback{ nullptr, +[](void*) noexcept {
        std::terminate();
      }}
    };
    context ctx{ el_ctx, chain_ctx };

    auto awaiter = co_awaitable.get_awaiter_for_context(ctx);

    awaiter.start_as_chain_root();

    while (!done)
    {
      el.do_current_pending_work();
    }

    return awaiter.await_resume();
  }
}