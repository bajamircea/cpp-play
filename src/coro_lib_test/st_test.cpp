#include "../test_lib/test.h"

#include "../coro_lib/st.h"

#include <string>

namespace
{
  TEST(st_test_trivial)
  {
    coro::st::context ctx;

    ctx.run(coro::deferred_co([]() -> coro::co<void> {
      co_return;
    }));
  }

  TEST(st_test_return)
  {
    coro::st::context ctx;

    int result = ctx.run(coro::deferred_co([]() -> coro::co<int> {
      co_return 42;
    }));

    ASSERT_EQ(42, result);
  }

  TEST(st_test_doze)
  {
    coro::st::context ctx;

    int result = ctx.run(coro::deferred_co([](coro::st::context & ctx) -> coro::co<int> {
      co_await ctx.doze();
      co_return 42;
    }, std::ref(ctx)));

    ASSERT_EQ(42, result);
  }

  TEST(st_test_sleep)
  {
    coro::st::context ctx;

    int result = ctx.run(coro::deferred_co([](coro::st::context & ctx) -> coro::co<int> {
      co_await ctx.sleep(std::chrono::seconds(0));
      co_return 42;
    }, std::ref(ctx)));

    ASSERT_EQ(42, result);
  }

  coro::co<std::string> bar(coro::st::context & ctx, int i)
  {
    co_await ctx.sleep(std::chrono::seconds(0));
    co_return std::to_string(i);
  }

  coro::co<std::string> foo(coro::st::context & ctx)
  {
    std::string x{ "start "};
    for (int i = 0 ; i < 3 ; ++i)
    {
      x += co_await bar(ctx, i);
    }
    x += " stop";
    co_return x;
  }

  TEST(st_test_timers)
  {
    coro::st::context ctx;

    std::string result = ctx.run(coro::deferred_co(foo, std::ref(ctx)));

    ASSERT_EQ("start 012 stop", result);
  }
} // anonymous namespace