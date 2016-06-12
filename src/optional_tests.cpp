#include "optional.h"

#include "test.h"

namespace
{
  using namespace optional;

  TEST(optional__use_with_int)
  {
    using pi = ptr<int>;

    pi a;
    ASSERT(a.is_empty());

    pi b{ a };
    ASSERT(b.is_empty());

    pi c{ 3 };
    ASSERT_FALSE(c.is_empty());
    ASSERT_EQ(3, c.get());

    pi d{ c };
    ASSERT_FALSE(d.is_empty());
    ASSERT_EQ(3, d.get());

    a = c;
    ASSERT_FALSE(a.is_empty());
    ASSERT_EQ(3, a.get());

    b = std::move(c);
    ASSERT(c.is_empty());
    ASSERT_FALSE(b.is_empty());
    ASSERT_EQ(3, b.get());

    pi e{ std::move(b) };
    ASSERT(b.is_empty());
    ASSERT_FALSE(e.is_empty());
    ASSERT_EQ(3, e.get());

    pi f{ 4 };
    f = std::move(e);
    ASSERT(e.is_empty());
    ASSERT_FALSE(f.is_empty());
    ASSERT_EQ(3, f.get());

    pi g;
    g = 5;
    ASSERT_FALSE(g.is_empty());
    ASSERT_EQ(5, g.get());

    pi h;
    h = std::move(6);
    ASSERT_FALSE(h.is_empty());
    ASSERT_EQ(6, h.get());
  }

  TEST(optional__with_incomplete_type)
  {
    struct node
    {
      explicit node(int x) : value{ x } {};
      int value;
      ptr<node> next;
    };

    node a(1);
    ASSERT_EQ(1, a.value);
    ASSERT(a.next.is_empty());

    a.next = node(2);
    ASSERT_FALSE(a.next.is_empty());
    ASSERT_EQ(2, a.next.get().value);
    ASSERT(a.next.get().next.is_empty());
  }
} // anonymous namespace
