#include "../test_lib/test.h"

#include "../coro_st_lib/promise_base.h"

#include <stdexcept>
#include <string>
#include <type_traits>

namespace
{
  TEST(promise_base_return_value)
  {
    coro_st::promise_base<std::string> x;
    x.return_value("42");

    auto result = x.get_result();
    static_assert(std::is_same_v<decltype(result), std::string>);

    ASSERT_EQ("42", result);
  }

  TEST(promise_base_exception)
  {
    coro_st::promise_base<std::string> x;
    try
    {
      throw std::runtime_error("42");
    }
    catch(const std::exception&)
    {
      x.unhandled_exception();
    }

    ASSERT_THROW_WHAT(x.get_result(), std::runtime_error, "42");
  }

  TEST(promise_base_return_void)
  {
    coro_st::promise_base<void> x;
    x.return_void();

    x.get_result();
  }

  TEST(promise_base_exception_void)
  {
    coro_st::promise_base<void> x;
    try
    {
      throw std::runtime_error("42");
    }
    catch(const std::exception&)
    {
      x.unhandled_exception();
    }

    ASSERT_THROW_WHAT(x.get_result(), std::runtime_error, "42");
  }
} // anonymous namespace