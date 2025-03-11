#include "../test_lib/test.h"

#include "../coro_lib/st_sleep.h"
#include "../coro_lib/st_run.h"

#include <stdexcept>
#include <string>

namespace
{
  TEST(st_sleep_trivial)
  {
    coro::st::run(coro::deferred_co<void>(
      coro::st::async_sleep, std::chrono::seconds(0)));
  }

  TEST(st_sleep_lambda)
  {
    int result = coro::st::run(coro::deferred_co<int>([](coro::st::context & ctx) -> coro::co<int> {
      co_await coro::st::async_sleep(ctx, std::chrono::seconds(0));
      co_return 42;
    }));

    ASSERT_EQ(42, result);
  }

  TEST(st_sleep_exception)
  {
    ASSERT_THROW_WHAT(coro::st::run(coro::deferred_co<void>([](coro::st::context & ctx) -> coro::co<void> {
      co_await coro::st::async_sleep(ctx, std::chrono::seconds(0));
      throw std::runtime_error("Ups!");
      co_return;
    })), std::runtime_error, "Ups!");
  }

  // coro::co<void> async_sleep_does_not_compile(coro::st::context & ctx)
  // {
  //   auto x = coro::st::async_sleep(ctx, std::chrono::seconds(0));
  //   co_await std::move(x);
  // }

  // coro::co<void> async_sleep_does_not_compile2(coro::st::context & ctx)
  // {
  //   coro::st::async_sleep(ctx, std::chrono::seconds(0));
  //   co_return;
  // }

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
    std::string result = coro::st::run(coro::deferred_co<std::string>(async_foo));

    ASSERT_EQ("start 012 stop", result);
  }
} // anonymous namespace