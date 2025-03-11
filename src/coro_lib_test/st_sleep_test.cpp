#include "../test_lib/test.h"

#include "../coro_lib/st_sleep.h"

#include <string>

namespace
{
  TEST(st_sleep_yield_trivial)
  {
    coro::st::scheduler s;

    s.run(coro::deferred_co<void>(coro::st::async_yield));
  }

  TEST(st_sleep_yield_lambda)
  {
    coro::st::scheduler s;

    int result = s.run(coro::deferred_co<int>([](coro::st::context & ctx) -> coro::co<int> {
      co_await coro::st::async_yield(ctx);
      co_return 42;
    }));

    ASSERT_EQ(42, result);
  }

  TEST(st_sleep_yield_exception)
  {
    coro::st::scheduler s;

    ASSERT_THROW_WHAT(s.run(coro::deferred_co<void>([](coro::st::context & ctx) -> coro::co<void> {
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

  TEST(st_sleep_sleep_trivial)
  {
    coro::st::scheduler s;

    s.run(coro::deferred_co<void>(
      coro::st::async_sleep, std::chrono::seconds(0)));
  }

  TEST(st_sleep_sleep_lambda)
  {
    coro::st::scheduler s;

    int result = s.run(coro::deferred_co<int>([](coro::st::context & ctx) -> coro::co<int> {
      co_await coro::st::async_sleep(ctx, std::chrono::seconds(0));
      co_return 42;
    }));

    ASSERT_EQ(42, result);
  }

  TEST(st_sleep_sleep_exception)
  {
    coro::st::scheduler s;

    ASSERT_THROW_WHAT(s.run(coro::deferred_co<void>([](coro::st::context & ctx) -> coro::co<void> {
      co_await coro::st::async_sleep(ctx, std::chrono::seconds(0));
      throw std::runtime_error("Ups!");
      co_return;
    })), std::runtime_error, "Ups!");
  }

  // // coro::co<void> async_sleep_does_not_compile(coro::st::context & ctx)
  // // {
  // //   auto x = coro::st::async_sleep(ctx, std::chrono::seconds(0));
  // //   co_await std::move(x);
  // // }

  // // coro::co<void> async_sleep_does_not_compile2(coro::st::context & ctx)
  // // {
  // //   coro::st::async_sleep(ctx, std::chrono::seconds(0));
  // //   co_return;
  // // }

  // coro::co<std::string> async_bar(coro::st::context & ctx, int i)
  // {
  //   co_await coro::st::async_sleep(ctx, std::chrono::seconds(0));
  //   co_return std::to_string(i);
  // }

  // coro::co<std::string> async_foo(coro::st::context & ctx)
  // {
  //   std::string x{ "start "};
  //   for (int i = 0 ; i < 3 ; ++i)
  //   {
  //     x += co_await async_bar(ctx, i);
  //   }
  //   x += " stop";
  //   co_return x;
  // }

  // TEST(st_sleep_timers)
  // {
  //   coro::st::context ctx;

  //   std::string result = ctx.run(coro::deferred_co(async_foo, std::ref(ctx)));

  //   ASSERT_EQ("start 012 stop", result);
  // }
} // anonymous namespace