#include "../test_lib/test.h"

#include "../coro_st_lib/event_loop.h"

namespace
{
  TEST(event_loop_trivial_ready_queue)
  {
    coro_st::event_loop el;

    bool called{ false };

    coro_st::ready_node n0;
    n0.cb = coro_st::make_function_callback<+[](bool& x) noexcept {
      x = true;
    }>(called);
    el.ready_queue_.push(&n0);

    auto sleep = el.do_current_pending_work();

    ASSERT_FALSE(sleep.has_value());
    ASSERT_TRUE(called);
  }

  TEST(event_loop_trivial_timer_heap)
  {
    coro_st::event_loop el;

    bool called{ false };

    auto now = std::chrono::steady_clock::now();

    coro_st::timer_node n0{ now + std::chrono::hours(24) };
    n0.cb = coro_st::callback(&called, +[](void*) noexcept {
      FAIL_TEST("Callback should not run");
    });
    el.timers_heap_.insert(&n0);

    coro_st::timer_node n1{ now };
    n1.cb = coro_st::make_function_callback<+[](bool& x) noexcept {
      x = true;
    }>(called);
    el.timers_heap_.insert(&n1);

    auto sleep = el.do_current_pending_work();

    ASSERT_TRUE(sleep.has_value());
    ASSERT_TRUE(called);
  }
} // anonymous namespace