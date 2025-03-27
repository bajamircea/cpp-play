#include "../test_lib/test.h"

#include "../coro_st_lib/event_loop_context.h"

namespace
{
  TEST(event_loop_context_ready_node)
  {
    coro_st::ready_queue q;
    coro_st::timer_heap h;

    coro_st::event_loop_context event_loop_context{ q, h };

    bool called{ false };

    coro_st::ready_node r0;
    r0.cb = coro_st::callback(&called, +[](void* x) noexcept {
      bool* p_called = reinterpret_cast<bool*>(x);
      *p_called = true;
    });

    event_loop_context.push_ready_node(r0);

    ASSERT_FALSE(q.empty());
    ASSERT_TRUE(h.empty());

    auto pq = q.pop();
    ASSERT_EQ(pq, &r0);

    pq->cb.invoke();

    ASSERT_TRUE(called);
    ASSERT_TRUE(q.empty());
  }

  TEST(event_loop_context_timer_node)
  {
    coro_st::ready_queue q;
    coro_st::timer_heap h;

    coro_st::event_loop_context event_loop_context{ q, h };

    bool called{ false };

    auto now = std::chrono::steady_clock::now();

    coro_st::timer_node t0{ now + std::chrono::seconds(1) };
    t0.cb = coro_st::callback(&t0, +[](void*) noexcept {});
    event_loop_context.insert_timer_node(t0);

    coro_st::timer_node t1{ now };
    t1.cb = coro_st::callback(&called, +[](void* x) noexcept {
      bool* p_called = reinterpret_cast<bool*>(x);
      *p_called = true;
    });
    event_loop_context.insert_timer_node(t1);

    ASSERT_TRUE(q.empty());
    ASSERT_FALSE(h.empty());
    ASSERT_EQ(&t1, h.min_node());

    auto ph = h.min_node();
    h.pop_min();
    ASSERT_EQ(ph, &t1);
    ASSERT_EQ(&t0, h.min_node());

    ph->cb.invoke();

    ASSERT_TRUE(called);

    event_loop_context.remove_timer_node(t0);
    ASSERT_TRUE(h.empty());
  }
} // anonymous namespace