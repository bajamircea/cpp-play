#include "../test_lib/test.h"

#include "../coro_st_lib/ready_queue.h"

namespace
{
  TEST(ready_queue_trivial)
  {
    coro_st::ready_queue q;

    ASSERT_TRUE(q.empty());

    bool called{ false };

    coro_st::ready_node n0;
    n0.fn = +[](void* x) noexcept {
      bool* p_called = reinterpret_cast<bool*>(x);
      *p_called = true;
    };
    n0.x = &called;
    q.push(&n0);
    ASSERT_FALSE(q.empty());
    ASSERT_EQ(nullptr, n0.next);

    coro_st::ready_node n1;
    q.push(&n1);
    ASSERT_EQ(&n1, n0.next);

    auto p = q.pop();
    ASSERT_EQ(p, &n0);

    p->fn(p->x);

    ASSERT_TRUE(called);
  }
} // anonymous namespace