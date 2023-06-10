#include "../test_lib/test.h"

#include "../cpp_util_lib/intrusive_queue.h"

namespace
{
  struct queue_node
  {
    queue_node * next;
    int value;
  };

  using queue = cpp_util::intrusive_queue<queue_node, &queue_node::next>;

  TEST(intrusive_queue_simple)
  {
    queue x;

    ASSERT_TRUE(x.empty());

    queue_node e0;
    e0.value = 40;
    x.push(&e0);
    ASSERT_FALSE(x.empty());
    ASSERT_EQ(nullptr, e0.next);

    queue_node e1;
    e1.value = 41;
    x.push(&e1);
    ASSERT_EQ(&e1, e0.next);
    ASSERT_EQ(nullptr, e1.next);

    queue_node e2;
    e2.value = 42;
    x.push(&e2);
    ASSERT_EQ(&e1, e0.next);
    ASSERT_EQ(&e2, e1.next);
    ASSERT_EQ(nullptr, e2.next);

    ASSERT_EQ(&e0, x.pop());
    ASSERT_EQ(&e1, x.pop());
    ASSERT_EQ(&e2, x.pop());

    ASSERT_TRUE(x.empty());

    ASSERT_EQ(nullptr, x.pop());
  }
}