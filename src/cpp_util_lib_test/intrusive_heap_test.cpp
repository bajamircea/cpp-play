#include "../test_lib/test.h"

#include "../cpp_util_lib/intrusive_heap.h"

#include <string>

namespace
{
  struct heap_node
  {
    std::string key;
    cpp_util::intrusive_heap_ptrs ptrs;
  };

  struct heap_node_compare
  {
    bool operator()(const heap_node & x, const heap_node & y)
    {
      return x.key < y.key;
    }
  };

  using heap_api = cpp_util::intrusive_heap<heap_node, &heap_node::ptrs, heap_node_compare>;

  TEST(intrusive_heap_simple)
  {
    cpp_util::intrusive_heap_object heap;

    heap_node foo_node{ "foo" };
    heap_node bar_node{ "bar" };
    heap_node buzz_node{ "buzz" };
    heap_node wozz_node{ "wozz" };

    ASSERT(heap_api::empty(heap));
    ASSERT_EQ(0, heap.size);
    ASSERT_EQ(nullptr, heap_api::min_node(heap));

    heap_api::insert(heap, &foo_node);

    ASSERT_FALSE(heap_api::empty(heap));
    ASSERT_EQ(1, heap.size);
    ASSERT_EQ(&foo_node, heap_api::min_node(heap));

    heap_api::insert(heap, &bar_node);

    ASSERT_FALSE(heap_api::empty(heap));
    ASSERT_EQ(2, heap.size);
    ASSERT_EQ(&bar_node, heap_api::min_node(heap));

    heap_api::insert(heap, &buzz_node);

    ASSERT_FALSE(heap_api::empty(heap));
    ASSERT_EQ(3, heap.size);
    ASSERT_EQ(&bar_node, heap_api::min_node(heap));

    heap_api::insert(heap, &wozz_node);

    ASSERT_FALSE(heap_api::empty(heap));
    ASSERT_EQ(4, heap.size);
    ASSERT_EQ(&bar_node, heap_api::min_node(heap));

    heap_api::pop_min(heap);

    ASSERT_FALSE(heap_api::empty(heap));
    ASSERT_EQ(3, heap.size);
    ASSERT_EQ(&buzz_node, heap_api::min_node(heap));

    heap_api::pop_min(heap);

    ASSERT_FALSE(heap_api::empty(heap));
    ASSERT_EQ(2, heap.size);
    ASSERT_EQ(&foo_node, heap_api::min_node(heap));

    heap_api::pop_min(heap);

    ASSERT_FALSE(heap_api::empty(heap));
    ASSERT_EQ(1, heap.size);
    ASSERT_EQ(&wozz_node, heap_api::min_node(heap));

    heap_api::pop_min(heap);

    ASSERT(heap_api::empty(heap));
    ASSERT_EQ(0, heap.size);
    ASSERT_EQ(nullptr, heap_api::min_node(heap));
  }
} // anonymous namespace