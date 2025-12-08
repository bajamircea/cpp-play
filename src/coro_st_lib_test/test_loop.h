#pragma once

#include "../coro_st_lib/event_loop_context.h"
#include "../coro_st_lib/context.h"
#include "../coro_st_lib/event_loop.h"
#include "../coro_st_lib/stop_util.h"
#include "../test_lib/test.h"

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

    // Not needed for the time being
    // void run_pending_work() noexcept
    // {
    //   while (!(el.ready_queue_.empty() && el.timers_heap_.empty()))
    //   {
    //     static_cast<void>(el.do_current_pending_work());
    //   }
    // }

    void run_one_ready(int count=1) noexcept
    {
      for (int i = 0; i < count; ++i)
      {
        auto* ready_node = el.ready_queue_.pop();
        ASSERT_NE(nullptr, ready_node);

        coro_st::callback cb = ready_node->cb;
        ASSERT_TRUE(cb.is_callable());
        cb.invoke();
      }
    }

    void run_one_timer() noexcept
    {
      auto* timer_node = el.timers_heap_.min_node();
      ASSERT_NE(nullptr, timer_node);
      el.timers_heap_.pop_min();

      coro_st::callback cb = timer_node->cb;
      ASSERT_TRUE(cb.is_callable());
      cb.invoke();
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