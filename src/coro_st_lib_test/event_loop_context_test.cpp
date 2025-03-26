#include "../test_lib/test.h"

#include "../coro_st_lib/event_loop_context.h"

namespace
{
  TEST(event_loop_context_timer_node_to_ready_node)
  {
    coro_st::ready_queue q;
    coro_st::timer_heap h;

    coro_st::event_loop_context event_loop_context{ q, h };

    bool called{ false };

    coro_st::ready_node r0;
    r0.fn = +[](void* x) noexcept {
      bool* p_called = reinterpret_cast<bool*>(x);
      *p_called = true;
    };
    r0.x = &called;

    auto now = std::chrono::steady_clock::now();

    coro_st::timer_node t0;
    t0.deadline = now + std::chrono::seconds(1);
    t0.fn = +[](void*) noexcept {};
    t0.x = &t0;
    event_loop_context.insert_timer_node(t0);

    struct sleep_data{
      coro_st::event_loop_context& event_loop_context;
      coro_st::ready_node& r;
    }y {event_loop_context, r0};

    coro_st::timer_node t1;
    t1.deadline = now;
    t1.fn = +[](void* x) noexcept {
      sleep_data* p = reinterpret_cast<sleep_data*>(x);
      p->event_loop_context.push_ready_node(p->r);
    };
    t1.x = &y;
    event_loop_context.insert_timer_node(t1);

    ASSERT_TRUE(q.empty());
    ASSERT_FALSE(h.empty());
    ASSERT_EQ(&t1, h.min_node());

    auto ph = h.min_node();
    h.pop_min();
    ASSERT_EQ(ph, &t1);

    ph->fn(ph->x);

    ASSERT_FALSE(q.empty());

    auto pq = q.pop();
    ASSERT_EQ(pq, &r0);

    pq->fn(pq->x);

    ASSERT_TRUE(called);
    ASSERT_TRUE(q.empty());

    event_loop_context.remove_timer_node(t0);
    ASSERT_TRUE(h.empty());
  }
} // anonymous namespace