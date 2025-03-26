#include "../test_lib/test.h"

#include "../coro_st_lib/timer_heap.h"

namespace
{
  TEST(timer_heap_trivial)
  {
    coro_st::timer_heap h;

    ASSERT_TRUE(h.empty());

    bool called{ false };

    auto now = std::chrono::steady_clock::now();

    coro_st::timer_node n0;
    n0.deadline = now + std::chrono::seconds(1);
    h.insert(&n0);
    ASSERT_FALSE(h.empty());
    ASSERT_EQ(&n0, h.min_node());

    coro_st::timer_node n1;
    n1.deadline = now;
    n1.fn = +[](void* x) noexcept {
      bool* p_called = reinterpret_cast<bool*>(x);
      *p_called = true;
    };
    n1.x = &called;
    h.insert(&n1);
    ASSERT_EQ(&n1, h.min_node());

    auto p = h.min_node();
    h.pop_min();
    ASSERT_EQ(p, &n1);

    p->fn(p->x);

    ASSERT_TRUE(called);
  }
} // anonymous namespace