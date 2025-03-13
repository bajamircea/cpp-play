#include "../test_lib/test.h"

#include "../coro_lib/st_yield.h"
#include "../coro_lib/st_run.h"

#include <stdexcept>
#include <string>

namespace
{
  TEST(st_yield_trivial)
  {
    coro::st::run(coro::deferred_co(coro::st::async_yield));
  }

  TEST(st_yield_lambda)
  {
    int result = coro::st::run(coro::deferred_co([](coro::st::context & ctx) -> coro::co<int> {
      co_await coro::st::async_yield(ctx);
      co_return 42;
    }));

    ASSERT_EQ(42, result);
  }

  TEST(st_yield_exception)
  {
    ASSERT_THROW_WHAT(coro::st::run(coro::deferred_co([](coro::st::context & ctx) -> coro::co<void> {
      co_await coro::st::async_yield(ctx);
      throw std::runtime_error("Ups!");
      co_return;
    })), std::runtime_error, "Ups!");
  }

  // coro::co<void> async_yield_does_not_compile(coro::st::context & ctx)
  // {
  //   auto x = coro::st::async_yield(ctx);
  //   co_await std::move(x);
  // }

  // coro::co<void> async_yield_does_not_compile2(coro::st::context & ctx)
  // {
  //   coro::st::async_yield(ctx);
  //   co_return;
  // }

  coro::co<std::string> async_bar(coro::st::context & ctx, int i)
  {
    co_await coro::st::async_yield(ctx);
    co_return std::to_string(i);
  }

  coro::co<std::string> async_foo(coro::st::context & ctx)
  {
    std::string x{ "start "};
    for (int i = 0 ; i < 3 ; ++i)
    {
      x += co_await async_bar(ctx, i);
    }
    x += " stop";
    co_return x;
  }

  TEST(st_yield_yields)
  {
    std::string result = coro::st::run(coro::deferred_co(async_foo));

    ASSERT_EQ("start 012 stop", result);
  }
} // anonymous namespace