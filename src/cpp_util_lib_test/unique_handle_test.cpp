#include "../test_lib/test.h"

#include "../cpp_util_lib/unique_handle.h"

#include <vector>

namespace
{
  std::vector<int> g_values_closed;

  struct simple_test_handle_traits
  {
    using handle_type = int;
    static constexpr auto invalid_value() noexcept { return -1; }
    static void close_handle(handle_type h) noexcept
    {
      g_values_closed.push_back(h);
    }
  };
  using simple_test_handle = cpp_util::unique_handle<simple_test_handle_traits>;

  static_assert(std::is_same_v<simple_test_handle::handle_type, int>);
  static_assert(std::is_same_v<simple_test_handle::traits_type, simple_test_handle_traits>);
  static_assert(-1 == simple_test_handle::traits_type::invalid_value());

  TEST(unique_handle_simple)
  {
    g_values_closed.clear();

    {
      simple_test_handle x(0);
      const simple_test_handle y(1);
      simple_test_handle z;

      ASSERT_EQ(0, x.get());
      ASSERT_EQ(1, y.get());
      ASSERT_EQ(-1, z.get());

      ASSERT_TRUE(x.is_valid());
      ASSERT_TRUE(y.is_valid());
      ASSERT_FALSE(z.is_valid());

      ASSERT_TRUE(x);
      ASSERT_TRUE(y);
      ASSERT_FALSE(z);
    }

    ASSERT_EQ((std::vector{1, 0}), g_values_closed);
  }

  TEST(unique_handle_move_construct)
  {
    g_values_closed.clear();

    {
      simple_test_handle x(0);
      simple_test_handle y(std::move(x));

      ASSERT_TRUE(g_values_closed.empty());
      ASSERT_EQ(-1, x.get());
      ASSERT_EQ(0, y.get());
    }

    ASSERT_EQ((std::vector{0}), g_values_closed);
  }

  TEST(unique_handle_move_assign)
  {
    g_values_closed.clear();

    {
      simple_test_handle x(0);
      simple_test_handle y;

      y = std::move(x);

      ASSERT_TRUE(g_values_closed.empty());
      ASSERT_EQ(-1, x.get());
      ASSERT_EQ(0, y.get());
    }

    ASSERT_EQ((std::vector{0}), g_values_closed);
  }

  TEST(unique_handle_reset)
  {
    g_values_closed.clear();

    {
      simple_test_handle x(0);
      x.reset(1);

      ASSERT_EQ((std::vector{0}), g_values_closed);
      ASSERT_EQ(1, x.get());
    }

    ASSERT_EQ((std::vector{0, 1}), g_values_closed);
  }

  TEST(unique_handle_release)
  {
    g_values_closed.clear();

    {
      simple_test_handle x(0);
      ASSERT_EQ(0, x.release());

      ASSERT_EQ(-1, x.get());
      ASSERT_FALSE(x.is_valid());
    }

    ASSERT_TRUE(g_values_closed.empty());
  }

  struct test_reinterpret_handle_traits
  {
    using handle_type = void *;
    static auto invalid_value() noexcept { return reinterpret_cast<handle_type>(-1); }
    static void close_handle(handle_type) noexcept
    {
      g_values_closed.push_back(42);
    }
  };
  using test_reinterpret_handle = cpp_util::unique_handle<test_reinterpret_handle_traits>;

  TEST(unique_handle_reinterpret_cast)
  {
    g_values_closed.clear();

    {
      test_reinterpret_handle x(nullptr);
    }

    ASSERT_EQ((std::vector{42}), g_values_closed);
  }

  struct test_bool_handle_traits
  {
    using handle_type = bool;
    static constexpr auto invalid_value() noexcept { return false; }
    static void close_handle(handle_type) noexcept
    {
      g_values_closed.push_back(43);
    }
  };
  using test_bool_handle = cpp_util::unique_handle<test_bool_handle_traits>;

  TEST(unique_handle_bool)
  {
    g_values_closed.clear();

    {
      test_bool_handle x(true);
    }

    ASSERT_EQ((std::vector{43}), g_values_closed);
  }

  enum class test_cleanup_action {
    off,
    on
  };
  struct test_enum_handle_traits
  {
    using handle_type = test_cleanup_action;
    static constexpr auto invalid_value() noexcept { return test_cleanup_action::off; }
    static void close_handle(handle_type) noexcept
    {
      g_values_closed.push_back(44);
    }
  };
  using test_enum_handle = cpp_util::unique_handle<test_enum_handle_traits>;

  TEST(unique_handle_enum)
  {
    g_values_closed.clear();

    {
      test_enum_handle x(test_cleanup_action::on);
    }

    ASSERT_EQ((std::vector{44}), g_values_closed);
  }

  struct test_bool_data_handle_traits
  {
    struct handle_type {
      int data;
      bool enabled;
    };
    static constexpr auto invalid_value() noexcept { return handle_type{0, false}; }
    static bool is_valid(const handle_type& h) noexcept
    {
      return h.enabled;
    }
    static void close_handle(const handle_type& h) noexcept
    {
      g_values_closed.push_back(h.data);
    }
  };
  // assert custom is_valid works
  static_assert(cpp_util::unique_handle_custom_is_valid_traits<test_bool_data_handle_traits>);
  using test_bool_data_handle = cpp_util::unique_handle<test_bool_data_handle_traits>;

  TEST(unique_handle_bool_data)
  {
    g_values_closed.clear();

    {
      test_bool_data_handle x(45, true);
    }

    ASSERT_EQ((std::vector{45}), g_values_closed);
  }
} // anonymous namespace