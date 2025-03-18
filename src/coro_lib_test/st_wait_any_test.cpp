#include "../test_lib/test.h"

#include "../coro_lib/st_wait_any.h"

#include "../coro_lib/co.h"
#include "../coro_lib/st_run.h"
#include "../coro_lib/st_yield.h"

#include <stdexcept>
#include <string>

namespace
{
  coro::co<int> async_0(coro::st::context & ctx)
  {
    co_await coro::st::async_yield(ctx);
    co_return 10;
  }

  coro::co<short> async_1(coro::st::context &)
  {
    co_return (short)20;
  }

  coro::co<int> async_2(coro::st::context &)
  {
    co_return 30;
  }

  coro::co<size_t> async_three(coro::st::context & ctx)
  {
    auto x = co_await coro::st::async_wait_any(ctx,
      async_0, async_1, async_2);
    co_return x.index + x.value;
  }

  TEST(st_wait_any_sample)
  {
    auto result = coro::st::run(async_three);

    ASSERT_EQ(21, result);
  }

  coro::co<size_t> async_wait_void(coro::st::context & ctx)
  {
    auto x = co_await coro::st::async_wait_any(ctx,
      [](coro::st::context& ctx) -> coro::co<void>{
        co_await coro::st::async_yield(ctx);
      },
      [](coro::st::context&) -> coro::co<void>{
        co_return;
      });
    co_return x.index;
  }

  TEST(st_wait_any_void)
  {
    auto result = coro::st::run(async_wait_void);

    ASSERT_EQ(1, result);
  }

  // TODO: add more tests
  // - exception
  // - cancellation
  // - direct resumption from await_suspend

  // TEST(st_yield_exception)
  // {
  //   ASSERT_THROW_WHAT(coro::st::run(coro::deferred_co([](coro::st::context & ctx) -> coro::co<void> {
  //     co_await coro::st::async_yield(ctx);
  //     throw std::runtime_error("Ups!");
  //     co_return;
  //   })), std::runtime_error, "Ups!");
  // }
} // anonymous namespace