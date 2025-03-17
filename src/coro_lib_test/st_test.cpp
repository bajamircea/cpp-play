#include "../test_lib/test.h"

#include "../coro_lib/st.h"

#include <string>
#include <stdexcept>

namespace
{
  TEST(st_trivial)
  {
    coro::st::run([](coro::st::context& ctx) -> coro::co<void> {
      co_return;
    });
  }

  TEST(st_return)
  {
    int result = coro::st::run([](coro::st::context& ctx) -> coro::co<int> {
      co_return 42;
    });

    ASSERT_EQ(42, result);
  }

  TEST(st_exception)
  {
    ASSERT_THROW_WHAT(coro::st::run([](coro::st::context& ctx) -> coro::co<void> {
      throw std::runtime_error("Ups!");
      co_return;
    }), std::runtime_error, "Ups!");
  }

  coro::co<int> async_foo(coro::st::context& ctx)
  {
    co_return 42;
  }

  TEST(st_return_call_co)
  {
    int result = coro::st::run(async_foo);

    ASSERT_EQ(42, result);
  }

  coro::co<int> async_bar(coro::st::context& ctx, const int& val)
  {
    co_return val + 1;
  }

  TEST(st_use_deferred_co)
  {
    int val = 40;

    auto fn = coro::deferred_co(async_bar, val + 1);

    int result = coro::st::run(fn);

    ASSERT_EQ(42, result);
  }

  coro::co<void> async_buzz(coro::st::context& ctx, int& val)
  {
    val = 42;
    co_return;
  }

  TEST(st_use_deferred_co_ref)
  {
    int val = 0;

    coro::st::run(coro::deferred_co(async_buzz, std::ref(val)));

    ASSERT_EQ(42, val);
  }

  coro::co<int> async_three(coro::st::context & ctx)
  {
    auto x = co_await coro::st::async_wait_any(ctx,
      [](coro::st::context& ctx) -> coro::co<void>{
        while (true)
        {
          co_await coro::st::async_yield(ctx);
        }
      },
      [](coro::st::context& ctx) -> coro::co<void>{
        co_await coro::st::async_suspend_forever(ctx);
      },
      [](coro::st::context& ctx) -> coro::co<void>{
        co_await coro::st::async_sleep(ctx, std::chrono::seconds(0));
      });
    co_return x.index;
  }

  TEST(st_wait_any_yield_sleep)
  {
    auto result = coro::st::run(async_three);

    ASSERT_EQ(2, result);
  }

} // anonymous namespace