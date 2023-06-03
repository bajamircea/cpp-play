#include "../test_lib/test.h"

#include "../cpp_util_lib/intrusive_heap.h"

#include <string>

namespace
{
  struct heap_node
  {
    std::string key;
    cpp_util::intrusive_heap_ptrs<heap_node> ptrs;
  };

  struct heap_node_compare
  {
    bool operator()(const heap_node & x, const heap_node & y)
    {
      return x.key < y.key;
    }
  };

  TEST(intrusive_heap_simple)
  {
    cpp_util::intrusive_heap<heap_node, &heap_node::ptrs, heap_node_compare> heap;

    heap_node foo_node{ "foo" };
    heap_node bar_node{ "bar" };
    heap_node buzz_node{ "buzz" };
    heap_node wozz_node{ "wozz" };

    ASSERT_TRUE(heap.empty());
    ASSERT_EQ(0, heap.size());
    ASSERT_EQ(nullptr, heap.min_node());

    heap.insert(&foo_node);

    ASSERT_FALSE(heap.empty());
    ASSERT_EQ(1, heap.size());
    ASSERT_EQ(&foo_node, heap.min_node());

    heap.insert(&bar_node);

    ASSERT_FALSE(heap.empty());
    ASSERT_EQ(2, heap.size());
    ASSERT_EQ(&bar_node, heap.min_node());

    heap.insert(&buzz_node);

    ASSERT_FALSE(heap.empty());
    ASSERT_EQ(3, heap.size());
    ASSERT_EQ(&bar_node, heap.min_node());

    heap.insert(&wozz_node);

    ASSERT_FALSE(heap.empty());
    ASSERT_EQ(4, heap.size());
    ASSERT_EQ(&bar_node, heap.min_node());

    heap.pop_min();

    ASSERT_FALSE(heap.empty());
    ASSERT_EQ(3, heap.size());
    ASSERT_EQ(&buzz_node, heap.min_node());

    heap.pop_min();

    ASSERT_FALSE(heap.empty());
    ASSERT_EQ(2, heap.size());
    ASSERT_EQ(&foo_node, heap.min_node());

    heap.pop_min();

    ASSERT_FALSE(heap.empty());
    ASSERT_EQ(1, heap.size());
    ASSERT_EQ(&wozz_node, heap.min_node());

    heap.pop_min();

    ASSERT_TRUE(heap.empty());
    ASSERT_EQ(0, heap.size());
    ASSERT_EQ(nullptr, heap.min_node());
  }
} // anonymous namespace