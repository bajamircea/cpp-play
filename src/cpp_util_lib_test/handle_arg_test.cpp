#include "../test_lib/test.h"

#include "../cpp_util_lib/handle_arg.h"

#include "../cpp_util_lib/unique_handle.h"

#include <vector>

namespace
{
  std::vector<int> g_values_used;

  struct test_handle_traits
  {
    using handle_type = int;
    static constexpr auto invalid_value() noexcept { return -1; }
    static void close(handle_type) noexcept {}
  };

  using test_handle = cpp_util::unique_handle<test_handle_traits>;

  using test_arg = cpp_util::handle_arg<test_handle>;

  void foo(test_arg x)
  {
    g_values_used.push_back(x);
  }

  TEST(handle_arg_simple)
  {
    g_values_used.clear();

    {
      test_handle x(0);
      foo(x);
      foo(1);
    }

    ASSERT_EQ((std::vector{0, 1}), g_values_used);
  }
} // anonymous namespace