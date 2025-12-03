#pragma once

#include "../coro_st_lib/event_loop_context.h"
#include "../coro_st_lib/context.h"
#include "../coro_st_lib/event_loop.h"
#include "../coro_st_lib/stop_util.h"

namespace coro_st_test
{
  struct test_loop
  {
    coro_st::stop_source stop_source{};
    bool result_ready { false };
    bool stopped { false };

    coro_st::event_loop el{};

    coro_st::event_loop_context el_ctx{ el.ready_queue_, el.timers_heap_ };
    coro_st::context ctx{
      el_ctx,
      stop_source.get_token(),
      coro_st::make_member_completion<
        &test_loop::on_result_ready,
        &test_loop::on_stopped
      >(this)
    };

    test_loop() noexcept = default;

    test_loop(const test_loop&) = delete;
    test_loop& operator=(const test_loop&) = delete;

    void run_pending_work() noexcept
    {
      while (!(el.ready_queue_.empty() && el.timers_heap_.empty()))
      {
        static_cast<void>(el.do_current_pending_work());
      }
    }

    void on_result_ready() noexcept
    {
      result_ready = true;
    }

    void on_stopped() noexcept
    {
      stopped = true;
    }
  };
}