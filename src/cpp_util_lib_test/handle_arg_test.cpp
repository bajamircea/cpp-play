#include "../test_lib/test.h"

#include "../cpp_util_lib/handle_arg.h"

#include <vector>

namespace
{
  std::vector<int> g_values_used;

  struct test_handle_traits : cpp_util::unique_handle_out_ptr_access
  {
    using handle_type = int;
    static constexpr auto invalid_value() noexcept { return -1; }
    static void close_handle(handle_type) noexcept {}
  };

  using test_handle = cpp_util::unique_handle<test_handle_traits>;
  using test_arg = cpp_util::handle_arg<test_handle_traits>;

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

  template<typename T>
  struct test_handle_template_traits : cpp_util::unique_handle_out_ptr_access
  {
    using handle_type = T;
    static constexpr auto invalid_value() noexcept { return -1; }
    static void close_handle(handle_type) noexcept {}
  };

  template<typename T>
  using test_template_handle = cpp_util::unique_handle<test_handle_template_traits<T>>;
  template<typename T>
  using test_template_arg = cpp_util::handle_arg<test_handle_template_traits<T>>;

  void foo_template(test_template_arg<int> x)
  {
    g_values_used.push_back(x);
  }

  TEST(handle_arg_template)
  {
    g_values_used.clear();

    {
      test_template_handle<int> x(0);
      foo_template(x);
      foo_template(1);
    }

    ASSERT_EQ((std::vector{0, 1}), g_values_used);
  }
} // anonymous namespace