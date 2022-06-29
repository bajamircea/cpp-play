#include "../test_lib/test.h"

#include "../cpp_util_lib/raii_with_invalid_value.h"

#include <vector>

namespace
{
  std::vector<int> g_values_closed;

  namespace detail
  {
    struct test_traits
    {
      using handle = int;

      static constexpr auto invalid_value = -1;

      static void close_handle(handle h) noexcept
      {
        g_values_closed.push_back(h);
      }
    };
  }

  using test_raii = cpp_util::raii_with_invalid_value<detail::test_traits>;

  TEST(raii_with_invalid_value_simple)
  {
    g_values_closed.clear();

    {
      test_raii x(0);
      const test_raii y(1);
      test_raii z;

      ASSERT_EQ(0, x.get());
      ASSERT_EQ(1, y.get());
      ASSERT_EQ(-1, z.get());

      ASSERT(x.is_valid());
      ASSERT(y.is_valid());
      ASSERT_FALSE(z.is_valid());

      ASSERT(x);
      ASSERT(y);
      ASSERT_FALSE(z);
    }

    ASSERT_EQ((std::vector{1, 0}), g_values_closed);
  }

  TEST(raii_with_invalid_value_move_construct)
  {
    g_values_closed.clear();

    {
      test_raii x(0);
      test_raii y(std::move(x));

      ASSERT(g_values_closed.empty());
      ASSERT_EQ(-1, x.get());
      ASSERT_EQ(0, y.get());
    }

    ASSERT_EQ((std::vector{0}), g_values_closed);
  }

  TEST(raii_with_invalid_value_move_assign)
  {
    g_values_closed.clear();

    {
      test_raii x(0);
      test_raii y;

      y = std::move(x);

      ASSERT(g_values_closed.empty());
      ASSERT_EQ(-1, x.get());
      ASSERT_EQ(0, y.get());
    }

    ASSERT_EQ((std::vector{0}), g_values_closed);
  }

  TEST(raii_with_invalid_value_reset)
  {
    g_values_closed.clear();

    {
      test_raii x(0);
      x.reset(1);

      ASSERT_EQ((std::vector{0}), g_values_closed);
      ASSERT_EQ(1, x.get());
    }

    ASSERT_EQ((std::vector{0, 1}), g_values_closed);
  }

  TEST(raii_with_invalid_value_release)
  {
    g_values_closed.clear();

    {
      test_raii x(0);
      ASSERT_EQ(0, x.release());

      ASSERT_EQ(-1, x.get());
      ASSERT_FALSE(x.is_valid());
    }

    ASSERT(g_values_closed.empty());
  }
} // anonymous namespace