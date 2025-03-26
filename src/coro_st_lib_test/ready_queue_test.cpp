#include "../test_lib/test.h"

#include "../coro_st_lib/ready_queue.h"

namespace
{
  TEST(ready_queue_trivial)
  {
    coro_st::ready_queue q;

    ASSERT_TRUE(q.empty());

    bool called{ false };

    coro_st::ready_node n;
    n.fn = +[](void* x) noexcept {
      bool* p_called = reinterpret_cast<bool*>(x);
      *p_called = true;
    };
    n.x = &called;
    q.push(&n);
    ASSERT_FALSE(q.empty());
    ASSERT_EQ(nullptr, n.next);

    auto pn = q.pop();
    pn->fn(pn->x);

    ASSERT_TRUE(called);
  }
} // anonymous namespace