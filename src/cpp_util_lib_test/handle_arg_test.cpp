#include "../test_lib/test.h"

#include "../cpp_util_lib/handle_arg.h"

#include "../cpp_util_lib/raii_with_invalid_value.h"

#include <vector>

namespace
{
  std::vector<int> g_values_used;

  namespace detail
  {
    struct test_traits
    {
      using handle = int;

      static constexpr auto invalid_value = -1;

      static void close_handle(handle) noexcept
      {
      }
    };
  }

  using test_raii = cpp_util::raii_with_invalid_value<detail::test_traits>;

  using test_arg = cpp_util::handle_arg<test_raii>;

  void foo(test_arg x)
  {
    g_values_used.push_back(x);
  }

  TEST(handle_arg_simple)
  {
    g_values_used.clear();

    {
      test_raii x(0);
      foo(x);
      foo(1);
    }

    ASSERT_EQ((std::vector{0, 1}), g_values_used);
  }
} // anonymous namespace