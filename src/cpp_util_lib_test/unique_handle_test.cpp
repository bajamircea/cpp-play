#include "../test_lib/test.h"

#include "../cpp_util_lib/unique_handle.h"

#include <vector>
#include <string_view>

namespace
{
  // used to track interractions with C APIs
  struct action
  {
    std::string_view descr;
    int value;

    constexpr bool operator==(const action&) const noexcept = default;
  };
  std::vector<action> g_actions;

  void c_api_close_handle(int h)
  {
      // if `*out` is invalid (-1) a C API would not close
      // we add it to g_actions for tracking/testing purposes
      g_actions.push_back({ "close", h });
  }

  bool c_api_out_ptr(int in_expected, int out_value, int* out)
  {
    if (nullptr == out)
    {
      return false;
    }
    ASSERT_EQ(in_expected, *out);

    if (out_value == 100)
    {
      // simulate error
      return false;
    }
    g_actions.push_back({ "out", out_value });
    *out = out_value;
    return true;
  }

  bool c_api_inout_ptr(int in_expected, int out_value, int* out)
  {
    if (nullptr == out)
    {
      return false;
    }
    ASSERT_EQ(in_expected, *out);

    if (out_value == 100)
    {
      // simulate error
      return false;
    }
    if (*out != out_value)
    {
      // if `*out` is invalid (-1) a C API would not close
      // we add it to g_actions for tracking/testing purposes
      g_actions.push_back({ "inout(in)", *out });
      g_actions.push_back({ "inout(out)", out_value });
      *out = out_value;
    }
    return true;
  }

  struct simple_test_handle_traits : cpp_util::unique_handle_inout_and_out_ptr_access
  {
    using handle_type = int;
    static constexpr auto invalid_value() noexcept { return -1; }
    static void close_handle(handle_type h) noexcept
    {
      c_api_close_handle(h);
    }
  };
  using simple_test_handle = cpp_util::unique_handle<simple_test_handle_traits>;

  static_assert(std::is_same_v<simple_test_handle::handle_type, int>);
  static_assert(std::is_same_v<simple_test_handle::traits_type, simple_test_handle_traits>);
  static_assert(-1 == simple_test_handle::traits_type::invalid_value());

  TEST(unique_handle_simple)
  {
    g_actions.clear();

    {
      simple_test_handle x(0);
      const simple_test_handle y(1);
      simple_test_handle z;

      ASSERT_EQ(0, x.get());
      ASSERT_EQ(1, y.get());
      // test that default constructed has the invalid handle value
      ASSERT_EQ(-1, z.get());
      ASSERT_TRUE(x.is_valid());
      ASSERT_TRUE(y.is_valid());
      ASSERT_FALSE(z.is_valid());

      ASSERT_TRUE(x);
      ASSERT_TRUE(y);
      ASSERT_FALSE(z);
    }

    std::vector<action> expected = {{"close", 1}, {"close", 0},};
    ASSERT_EQ(expected, g_actions);
  }

  TEST(unique_handle_move_construct)
  {
    g_actions.clear();

    {
      simple_test_handle x(0);
      simple_test_handle y(std::move(x));

      ASSERT_TRUE(g_actions.empty());
      // test that the moved from value is the invalid handle value
      ASSERT_EQ(-1, x.get());
      ASSERT_EQ(0, y.get());
    }

    std::vector<action> expected = {{"close", 0},};
    ASSERT_EQ(expected, g_actions);
  }

  TEST(unique_handle_move_assign)
  {
    g_actions.clear();

    {
      simple_test_handle x(0);
      simple_test_handle y;

      y = std::move(x);

      ASSERT_TRUE(g_actions.empty());
      ASSERT_EQ(-1, x.get());
      ASSERT_EQ(0, y.get());
    }

    std::vector<action> expected = {{"close", 0},};
    ASSERT_EQ(expected, g_actions);
  }

  TEST(unique_handle_reset)
  {
    g_actions.clear();

    {
      simple_test_handle x(0);
      x.reset(1);

      std::vector<action> before_scope_exit = {{"close", 0},};
      ASSERT_EQ(before_scope_exit, g_actions);
      ASSERT_EQ(1, x.get());
    }

    std::vector<action> expected = {{"close", 0}, {"close", 1},};
    ASSERT_EQ(expected, g_actions);
  }

  TEST(unique_handle_release)
  {
    g_actions.clear();

    {
      simple_test_handle x(0);
      ASSERT_EQ(0, x.release());

      ASSERT_EQ(-1, x.get());
      ASSERT_FALSE(x.is_valid());
    }

    ASSERT_TRUE(g_actions.empty());
  }

  TEST(unique_handle_out_ptr_trivial)
  {
    g_actions.clear();

    {
      simple_test_handle x;
      ASSERT_TRUE(c_api_out_ptr(-1, 42, x.out_ptr()));

      std::vector<action> before_scope_exit = {{"out", 42},};
      ASSERT_EQ(before_scope_exit, g_actions);
      ASSERT_EQ(42, x.get());
    }

    std::vector<action> expected = {{"out", 42}, {"close", 42},};
    ASSERT_EQ(expected, g_actions);
  }

  TEST(unique_handle_out_ptr_already_has_value)
  {
    g_actions.clear();

    {
      simple_test_handle x(43);
      ASSERT_TRUE(c_api_out_ptr(-1, 42, x.out_ptr()));

      std::vector<action> before_scope_exit = {{"close", 43}, {"out", 42},};
      ASSERT_EQ(before_scope_exit, g_actions);
      ASSERT_EQ(42, x.get());
    }

    std::vector<action> expected = {{"close", 43}, {"out", 42}, {"close", 42},};
    ASSERT_EQ(expected, g_actions);
  }

  TEST(unique_handle_out_ptr_already_has_value_and_fail)
  {
    g_actions.clear();

    {
      simple_test_handle x(43);
      ASSERT_FALSE(c_api_out_ptr(-1, 100, x.out_ptr()));

      std::vector<action> before_scope_exit = {{"close", 43},};
      ASSERT_EQ(before_scope_exit, g_actions);
      ASSERT_EQ(-1, x.get());
      ASSERT_FALSE(x.is_valid());
    }

    std::vector<action> expected = {{"close", 43},};
    ASSERT_EQ(expected, g_actions);
  }

  TEST(unique_handle_out_ptr_no_proxy)
  {
    g_actions.clear();

    {
      simple_test_handle x;
      // tests that out_ptr does not return a temporary
      // proxy object like std::out_ptr, which would
      // live too long and go throgh the `else` branch
      if(c_api_out_ptr(-1, 42, x.out_ptr()) && x.is_valid())
      {
        std::vector<action> before_scope_exit = {{"out", 42},};
        ASSERT_EQ(before_scope_exit, g_actions);
        ASSERT_EQ(42, x.get());
      }
      else
      {
        FAIL_TEST("should not reach");
      }
    }

    std::vector<action> expected = {{"out", 42}, {"close", 42},};
    ASSERT_EQ(expected, g_actions);
  }

  TEST(unique_handle_inout_ptr_trivial)
  {
    g_actions.clear();

    {
      simple_test_handle x;
      ASSERT_TRUE(c_api_inout_ptr(-1, 42, x.inout_ptr()));

      std::vector<action> before_scope_exit = {{"inout(in)", -1}, {"inout(out)", 42},};
      ASSERT_EQ(before_scope_exit, g_actions);
      ASSERT_EQ(42, x.get());
    }

    std::vector<action> expected = {{"inout(in)", -1}, {"inout(out)", 42}, {"close", 42},};
    ASSERT_EQ(expected, g_actions);
  }

  TEST(unique_handle_inout_ptr_already_has_value)
  {
    g_actions.clear();

    {
      simple_test_handle x(43);
      ASSERT_TRUE(c_api_inout_ptr(43, 42, x.inout_ptr()));

      std::vector<action> before_scope_exit = {{"inout(in)", 43}, {"inout(out)", 42},};
      ASSERT_EQ(before_scope_exit, g_actions);
      ASSERT_EQ(42, x.get());
    }

    std::vector<action> expected = {{"inout(in)", 43}, {"inout(out)", 42}, {"close", 42},};
    ASSERT_EQ(expected, g_actions);
  }

  TEST(unique_handle_inout_ptr_already_has_value_and_fail)
  {
    g_actions.clear();

    {
      simple_test_handle x(43);
      ASSERT_FALSE(c_api_inout_ptr(43, 100, x.inout_ptr()));

      std::vector<action> before_scope_exit = {};
      ASSERT_EQ(before_scope_exit, g_actions);
      ASSERT_EQ(43, x.get());
    }

    std::vector<action> expected = {{"close", 43},};
    ASSERT_EQ(expected, g_actions);
  }

  TEST(unique_handle_inout_ptr_no_proxy)
  {
    g_actions.clear();

    {
      simple_test_handle x;
      // tests that inout_ptr does not return a temporary
      // proxy object like std::inout_ptr, which would
      // live too long and go throgh the `else` branch
      if(c_api_inout_ptr(-1, 42, x.inout_ptr()) && x.is_valid())
      {
        std::vector<action> before_scope_exit = {{"inout(in)", -1}, {"inout(out)", 42},};
        ASSERT_EQ(before_scope_exit, g_actions);
        ASSERT_EQ(42, x.get());
      }
      else
      {
        FAIL_TEST("should not reach");
      }
    }

    std::vector<action> expected = {{"inout(in)", -1}, {"inout(out)", 42}, {"close", 42},};
    ASSERT_EQ(expected, g_actions);
  }

  TEST(unique_handle_inout_ref_trivial)
  {
    g_actions.clear();

    simple_test_handle x;
    ASSERT_EQ(x.inout_ptr(), &x.inout_ref());
  }

  struct test_reinterpret_handle_traits :  cpp_util::unique_handle_out_ptr_access
  {
    using handle_type = void *;
    // not using constexpr for invalid_value()
    // because incompatible with the reintepret_cast
    // see commented static_asserts below
    static auto invalid_value() noexcept { return reinterpret_cast<handle_type>(-1); }
    static void close_handle(handle_type) noexcept
    {
      g_actions.push_back({"close void*", 42});
    }
  };
  using test_reinterpret_handle = cpp_util::unique_handle<test_reinterpret_handle_traits>;

  TEST(unique_handle_reinterpret_cast)
  {
    // this does not compile even by adding constexpr to the traits invalid_value
    // because of the reinterpret_cast
    //static_assert(nullptr != test_reinterpret_handle_traits::invalid_value());

    g_actions.clear();

    {
      test_reinterpret_handle x(nullptr);
    }

    std::vector<action> expected = {{"close void*", 42},};
    ASSERT_EQ(expected, g_actions);
  }

  struct test_reinterpret_handle_is_valid_traits : cpp_util::unique_handle_out_ptr_access
  {
    using handle_type = void *;
    // not using constexpr for invalid_value() or is_valid()
    // because incompatible with the reintepret_cast
    // see commented static_asserts below
    static auto invalid_value() noexcept { return reinterpret_cast<handle_type>(-1); }
    static auto is_valid(handle_type h) noexcept {
      return (h != nullptr) && (h != reinterpret_cast<handle_type>(-1));
    }
    static void close_handle(handle_type) noexcept
    {
      g_actions.push_back({"close void*", 43});
    }
  };
  static_assert(cpp_util::unique_handle_custom_is_valid_traits<test_reinterpret_handle_is_valid_traits>);
  using test_reinterpret_handle_is_valid = cpp_util::unique_handle<test_reinterpret_handle_is_valid_traits>;

  TEST(unique_handle_reinterpret_cast_with_is_valid)
  {
    // these do not compile even by adding constexpr to the traits methods
    // because of the reinterpret_cast
    // static_assert(!test_reinterpret_handle_is_valid_traits::is_valid(nullptr));
    // static_assert(!test_reinterpret_handle_is_valid_traits::is_valid(
    //  test_reinterpret_handle_is_valid_traits::invalid_value()));
    // static_assert(nullptr != test_reinterpret_handle_is_valid_traits::invalid_value());

    g_actions.clear();

    {
      test_reinterpret_handle_is_valid x(reinterpret_cast<void *>(100));
    }

    std::vector<action> expected = {{"close void*", 43},};
    ASSERT_EQ(expected, g_actions);
  }

  struct test_bool_handle_traits : cpp_util::unique_handle_basic_access
  {
    using handle_type = bool;
    static constexpr auto invalid_value() noexcept { return false; }
    static void close_handle(handle_type h) noexcept
    {
      g_actions.push_back({"close bool", (false == h)? 0: 1});
    }
  };
  using test_bool_handle = cpp_util::unique_handle<test_bool_handle_traits>;

  TEST(unique_handle_bool)
  {
    g_actions.clear();

    {
      test_bool_handle x(true);
    }

    std::vector<action> expected = {{"close bool", 1},};
    ASSERT_EQ(expected, g_actions);
  }

  enum class test_cleanup_action {
    off,
    on
  };
  struct test_enum_handle_traits : cpp_util::unique_handle_basic_access
  {
    using handle_type = test_cleanup_action;
    static constexpr auto invalid_value() noexcept { return test_cleanup_action::off; }
    static void close_handle(handle_type h) noexcept
    {
      g_actions.push_back({"close enum", (test_cleanup_action::off == h)? 0: 1});
    }
  };
  using test_enum_handle = cpp_util::unique_handle<test_enum_handle_traits>;

  TEST(unique_handle_enum)
  {
    g_actions.clear();

    {
      test_enum_handle x(test_cleanup_action::on);
    }

    std::vector<action> expected = {{"close enum", 1},};
    ASSERT_EQ(expected, g_actions);
  }

  struct test_bool_data_handle_traits : cpp_util::unique_handle_basic_access
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
      g_actions.push_back({"close data", h.data});
    }
  };
  // assert custom is_valid works
  static_assert(cpp_util::unique_handle_custom_is_valid_traits<test_bool_data_handle_traits>);
  using test_bool_data_handle = cpp_util::unique_handle<test_bool_data_handle_traits>;

  TEST(unique_handle_bool_data)
  {
    g_actions.clear();

    {
      test_bool_data_handle x(45, true);
    }

    std::vector<action> expected = {{"close data", 45},};
    ASSERT_EQ(expected, g_actions);
  }

  struct some_c_struct
  {
    int lots;
    int of;
    int data;
  };

  bool c_api_some_c_struct_out_ptr(
    int in_lots_expected, int out_lots_value,
    int in_of_expected, int out_of_value,
    int in_data_expected, int out_data_value,
    some_c_struct* out)
  {
    if (nullptr == out)
    {
      return false;
    }
    ASSERT_EQ(in_lots_expected, out->lots);
    ASSERT_EQ(in_of_expected, out->of);
    ASSERT_EQ(in_data_expected, out->data);

    if (out_lots_value == 100)
    {
      // simulate error
      return false;
    }
    g_actions.push_back({ "out c_struct",
      out_lots_value + out_of_value + out_data_value });
    out->lots = out_lots_value;
    out->of = out_of_value;
    out->data = out_data_value;
    return true;
  }

  struct test_c_struct_data_handle_traits :  cpp_util::unique_handle_out_ptr_access
  {
    using handle_type = some_c_struct;
    static constexpr auto invalid_value() noexcept
    {
      return handle_type{};
    }
    static bool is_valid(const handle_type& h) noexcept
    {
      return (0!= h.lots) || (0 != h.of);
    }
    static void close_handle(const handle_type& h) noexcept
    {
      g_actions.push_back({"close c_struct", h.lots + h.of});
    }
  };
  // assert custom is_valid works
  static_assert(cpp_util::unique_handle_custom_is_valid_traits<test_c_struct_data_handle_traits>);
  using test_c_struct_data_handle = cpp_util::unique_handle<test_c_struct_data_handle_traits>;

  TEST(unique_handle_c_struct_data_trivial)
  {
    g_actions.clear();

    {
      test_c_struct_data_handle x;

      ASSERT_TRUE(c_api_some_c_struct_out_ptr(0, 40, 0, 2, 0, 1, x.out_ptr()));
    }

    std::vector<action> expected = {{"out c_struct", 43}, {"close c_struct", 42},};
    ASSERT_EQ(expected, g_actions);
  }

  TEST(unique_handle_c_struct_data_already_has_value)
  {
    g_actions.clear();

    {
      test_c_struct_data_handle x;

      ASSERT_TRUE(c_api_some_c_struct_out_ptr(0, 40, 0, 2, 0, 1, x.out_ptr()));
      ASSERT_TRUE(c_api_some_c_struct_out_ptr(0, 30, 0, 2, 0, 1, x.out_ptr()));
    }

    std::vector<action> expected = {
      {"out c_struct", 43}, {"close c_struct", 42},
      {"out c_struct", 33}, {"close c_struct", 32},
    };
    ASSERT_EQ(expected, g_actions);
  }


  TEST(unique_handle_c_struct_data_already_has_value_and_fail)
  {
    g_actions.clear();

    {
      test_c_struct_data_handle x;

      ASSERT_TRUE(c_api_some_c_struct_out_ptr(0, 40, 0, 2, 0, 1, x.out_ptr()));
      ASSERT_FALSE(c_api_some_c_struct_out_ptr(0, 100, 0, 0, 0, 0, x.out_ptr()));

      std::vector<action> before_scope_exit = {
        {"out c_struct", 43}, {"close c_struct", 42},
      };
      ASSERT_EQ(before_scope_exit, g_actions);
    }

    std::vector<action> expected = {
      {"out c_struct", 43}, {"close c_struct", 42},
    };
    ASSERT_EQ(expected, g_actions);
  }
} // anonymous namespace