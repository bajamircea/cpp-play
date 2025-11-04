#include "../test_lib/test.h"

#include "../coro_st_lib/run.h"

#include "../coro_st_lib/co.h"
#include "../coro_st_lib/just_stopped.h"

namespace
{
  coro_st::co<int> async_foo()
  {
    co_return 42;
  };

  TEST(run_co_return_int)
  {
    int result = coro_st::run(async_foo()).value();

    ASSERT_EQ(42, result);
  }

  TEST(run_lambda_return_int)
  {
    auto async_lambda = []() -> coro_st::co<int> {
      co_return 42;
    };
    int result = coro_st::run(async_lambda()).value();

    ASSERT_EQ(42, result);
  }

  TEST(run_lambda_return_void)
  {
    auto async_lambda = []() -> coro_st::co<void> {
      co_return;
    };
    coro_st::run(async_lambda()).value();
  }

  TEST(run_lambda_exception)
  {
    auto async_lambda = []() -> coro_st::co<void> {
      throw std::runtime_error("Ups!");
      co_return;
    };

    ASSERT_THROW_WHAT(coro_st::run(async_lambda()),
      std::runtime_error, "Ups!");
  }

  coro_st::co<void> async_buzz_ref(int& val)
  {
    val = 42;
    co_return;
  }

  TEST(run_co_ref)
  {
    int val = 0;

    coro_st::run(async_buzz_ref(val)).value();

    ASSERT_EQ(42, val);
  }

  coro_st::co<int> async_buzz_const_ref(const int& val)
  {
    co_return val - 1;
  }

  TEST(run_co_const_ref)
  {
    int val = 42;

    int result = coro_st::run(async_buzz_const_ref(val + 1)).value();

    ASSERT_EQ(val, result);
  }

  coro_st::co<std::string> async_chain_leaf(int& i)
  {
    ++i;
    co_return std::to_string(i);
  }

  coro_st::co<std::string> async_chain_root()
  {
    std::string x{ "start " };
    for (int i = 0 ; i < 3 ;)
    {
      x += co_await async_chain_leaf(i);
    }
    x += " stop";
    co_return x;
  }

  TEST(run_chain)
  {
    std::string result = coro_st::run(async_chain_root()).value();

    ASSERT_EQ("start 123 stop", result);
  }

  TEST(run_stopped)
  {
    auto result = coro_st::run(coro_st::async_just_stopped());

    ASSERT_FALSE(result.has_value());
  }
} // anonymous namespace