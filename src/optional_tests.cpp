#include "optional.h"

#include "test.h"

namespace
{
  using namespace optional;

  TEST(optional__use_with_int)
  {
    using pi = ptr<int>;

    {
      pi x;
      ASSERT(x.is_empty());
      ASSERT_THROW(x.at(), std::bad_cast);
    }

    {
      pi x{ 3 };
      ASSERT_FALSE(x.is_empty());
      ASSERT_EQ(3, x.get());
      ASSERT_EQ(3, x.at());
    }

    {
      pi y;
      pi x{ y };
      ASSERT(x.is_empty());
    }

    {
      pi y{ 3 };
      pi x{ y };
      ASSERT_FALSE(x.is_empty());
      ASSERT_EQ(3, x.get());
    }

    {
      pi y{ 3 };
      pi x;
      x = y;
      ASSERT_FALSE(x.is_empty());
      ASSERT_EQ(3, x.get());
    }

    {
      pi y{ 3 };
      pi x{ 4 };
      x = y;
      ASSERT_FALSE(x.is_empty());
      ASSERT_EQ(3, x.get());
    }

    {
      pi y{ 3 };
      pi x{ std::move(y) };
      ASSERT(y.is_empty());
      ASSERT_FALSE(x.is_empty());
      ASSERT_EQ(3, x.get());
    }

    {
      pi y{ 3 };
      pi x;
      x = std::move(y);
      ASSERT(y.is_empty());
      ASSERT_FALSE(x.is_empty());
      ASSERT_EQ(3, x.get());
    }

    {
      pi x;
      x = 5;
      ASSERT_FALSE(x.is_empty());
      ASSERT_EQ(5, x.get());
    }

    {
      pi x{ 4 };
      x = 5;
      ASSERT_FALSE(x.is_empty());
      ASSERT_EQ(5, x.get());
    }

    {
      pi x;
      x = std::move(6);
      ASSERT_FALSE(x.is_empty());
      ASSERT_EQ(6, x.get());
    }

    {
      pi x{ 2 };
      x = std::move(6);
      ASSERT_FALSE(x.is_empty());
      ASSERT_EQ(6, x.get());
    }

    {
      pi x{ 3 };
      x.clear();
      ASSERT(x.is_empty());
    }

    {
      const pi x{ 3 };
      ASSERT_FALSE(x.is_empty());
      ASSERT_EQ(3, x.get());
      ASSERT_EQ(3, x.at());
    }
  }

  TEST(optional__with_incomplete_type)
  {
    struct node
    {
      explicit node(int x) : value{ x } {};
      int value;
      ptr<node> next;
    };

    node a{ 1 };
    ASSERT_EQ(1, a.value);
    ASSERT(a.next.is_empty());

    a.next = node(2);
    ASSERT_FALSE(a.next.is_empty());
    ASSERT_EQ(2, a.next.get().value);
    ASSERT(a.next.get().next.is_empty());
  }
} // anonymous namespace
