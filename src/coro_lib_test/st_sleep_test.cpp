#include "../test_lib/test.h"

#include "../coro_lib/st_sleep.h"

#include <string>

namespace
{
  TEST(st_sleep_defer)
  {
    coro::st::context ctx;

    int result = ctx.run(coro::deferred_co([](coro::st::context & ctx) -> coro::co<int> {
      co_await coro::st::async_defer(ctx);
      co_return 42;
    }, std::ref(ctx)));

    ASSERT_EQ(42, result);
  }

  TEST(st_sleep_defer_exception)
  {
    coro::st::context ctx;

    ASSERT_THROW_WHAT(ctx.run(coro::deferred_co([](coro::st::context & ctx) -> coro::co<void> {
      co_await coro::st::async_defer(ctx);
      throw std::runtime_error("Ups!");
      co_return;
    }, std::ref(ctx))), std::runtime_error, "Ups!");
  }

  TEST(st_sleep_sleep)
  {
    coro::st::context ctx;

    int result = ctx.run(coro::deferred_co([](coro::st::context & ctx) -> coro::co<int> {
      co_await coro::st::async_sleep(ctx, std::chrono::seconds(0));
      co_return 42;
    }, std::ref(ctx)));

    ASSERT_EQ(42, result);
  }

  TEST(st_sleep_sleep_exception)
  {
    coro::st::context ctx;

    ASSERT_THROW_WHAT(ctx.run(coro::deferred_co([](coro::st::context & ctx) -> coro::co<void> {
      co_await coro::st::async_sleep(ctx, std::chrono::seconds(0));
      throw std::runtime_error("Ups!");
      co_return;
    }, std::ref(ctx))), std::runtime_error, "Ups!");
  }

  coro::co<std::string> async_bar(coro::st::context & ctx, int i)
  {
    co_await coro::st::async_sleep(ctx, std::chrono::seconds(0));
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

  TEST(st_sleep_timers)
  {
    coro::st::context ctx;

    std::string result = ctx.run(coro::deferred_co(async_foo, std::ref(ctx)));

    ASSERT_EQ("start 012 stop", result);
  }
} // anonymous namespace