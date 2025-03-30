#include "../test_lib/test.h"

#include "../coro_st_lib/run.h"

#include "../coro_st_lib/co.h"

namespace
{
  coro_st::co<int> async_foo()
  {
    co_return 42;
  };

  TEST(run_co_return_int)
  {
    int result = coro_st::run(async_foo());

    ASSERT_EQ(42, result);
  }

  TEST(run_lambda_return_int)
  {
    auto async_lambda = []() -> coro_st::co<int> {
      co_return 42;
    };
    int result = coro_st::run(async_lambda());

    ASSERT_EQ(42, result);
  }

  TEST(run_lambda_return_void)
  {
    auto async_lambda = []() -> coro_st::co<void> {
      co_return;
    };
    coro_st::run(async_lambda());
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

  coro_st::co<void> async_buzz(int& val)
  {
    val = 42;
    co_return;
  }

  TEST(run_co_ref)
  {
    int val = 0;

    coro_st::run(async_buzz(val));

    ASSERT_EQ(42, val);
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
    std::string result = coro_st::run(async_chain_root());

    ASSERT_EQ("start 123 stop", result);
  }
} // anonymous namespace