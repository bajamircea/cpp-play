#include "../test_lib/test.h"

#include "../cpp_util_lib/intrusive_list.h"

namespace
{
  struct list_node
  {
    int value;
    cpp_util::intrusive_list_ptrs<list_node> ptrs;
  };

  using lst = cpp_util::intrusive_list<list_node, &list_node::ptrs>;

  TEST(empty_list)
  {
    lst x;
    ASSERT(x.empty());
    ASSERT_EQ(0, x.size());

    int i{ 0 };
    for ([[maybe_unused]]list_node & elem : x)
    {
      ++i;
    }
    ASSERT_EQ(0, i);
  }

  TEST(one_elem)
  {
    lst x;
    list_node y;
    y.value = 42;
    ASSERT_EQ(nullptr, y.ptrs.next);
    ASSERT_EQ(nullptr, y.ptrs.prev);

    x.push_back(&y);
    ASSERT_EQ(nullptr, y.ptrs.next);
    ASSERT_EQ(nullptr, y.ptrs.prev);

    ASSERT_FALSE(x.empty());
    ASSERT_EQ(1, x.size());

    int i{ 0 };
    for (list_node & elem : x)
    {
      ASSERT_EQ(42, elem.value);
      ++i;
    }
    ASSERT_EQ(1, i);

    x.remove(&y);
    ASSERT_EQ(nullptr, y.ptrs.next);
    ASSERT_EQ(nullptr, y.ptrs.prev);

    ASSERT(x.empty());
    ASSERT_EQ(0, x.size());

    i = 0;
    for ([[maybe_unused]]list_node & elem : x)
    {
      ++i;
    }
    ASSERT_EQ(0, i);
  }

  TEST(two_elem)
  {
    lst x;
    list_node y;
    y.value = 42;
    ASSERT_EQ(nullptr, y.ptrs.next);
    ASSERT_EQ(nullptr, y.ptrs.prev);

    list_node z;
    z.value = 43;
    ASSERT_EQ(nullptr, z.ptrs.next);
    ASSERT_EQ(nullptr, z.ptrs.prev);

    x.push_back(&y);
    x.push_back(&z);

    ASSERT_NE(nullptr, y.ptrs.next);
    ASSERT_EQ(nullptr, y.ptrs.prev);
    ASSERT_EQ(nullptr, z.ptrs.next);
    ASSERT_NE(nullptr, z.ptrs.prev);

    ASSERT_FALSE(x.empty());
    ASSERT_EQ(2, x.size());

    int i{ 0 };
    for (list_node & elem : x)
    {
      ASSERT_EQ(42 + i, elem.value);
      ++i;
    }
    ASSERT_EQ(2, i);

    x.remove(&y);
    ASSERT_EQ(nullptr, y.ptrs.next);
    ASSERT_EQ(nullptr, y.ptrs.prev);

    x.remove(&z);
    ASSERT_EQ(nullptr, z.ptrs.next);
    ASSERT_EQ(nullptr, z.ptrs.prev);

    ASSERT(x.empty());
    ASSERT_EQ(0, x.size());

    i = 0;
    for ([[maybe_unused]]list_node & elem : x)
    {
      ++i;
    }
    ASSERT_EQ(0, i);
  }
}