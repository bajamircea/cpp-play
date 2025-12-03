#pragma once

#include "event_loop.h"
#include "event_loop_context.h"
#include "context.h"
#include "coro_type_traits.h"
#include "value_type_traits.h"

#include <optional>
#include <thread>

namespace coro_st
{
  template<is_co_task CoTask>
  auto run(CoTask co_task)
    -> std::optional<value_type_traits::value_type_t<co_task_result_t<CoTask>>>
  {
    stop_source main_stop_source;
    struct completion_flags
    {
      bool done { false };
      bool stopped { false };
    };
    completion_flags cf;

    event_loop el;

    event_loop_context el_ctx{ el.ready_queue_, el.timers_heap_ };
    context ctx{
      el_ctx,
      main_stop_source.get_token(),
      make_function_completion<
        +[](completion_flags& x) noexcept {
          x.done = true;
        },
        +[](completion_flags& x) noexcept {
          x.done = true;
          x.stopped = true;
        }
        >(cf)
    };

    auto co_awaiter = co_task.get_work().get_awaiter(ctx);

    co_awaiter.start();

    while (!cf.done)
    {
      auto sleep_time = el.do_current_pending_work();
      if (sleep_time.has_value())
      {
        std::this_thread::sleep_for(*sleep_time);
      }
    }

    if (cf.stopped)
    {
      return std::nullopt;
    }

    if constexpr (std::is_same_v<void, co_task_result_t<CoTask>>)
    {
      co_awaiter.await_resume();
      return void_result{};
    }
    else
    {
      return co_awaiter.await_resume();
    }
  }
}