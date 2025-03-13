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

  coro::co<short> async_1(coro::st::context & ctx)
  {
    co_return 20;
  }

  coro::co<int> async_both(coro::st::context & ctx)
  {
    auto x = co_await coro::st::async_wait_any(ctx,
      coro::deferred_co(async_0), coro::deferred_co(async_1));
    co_return x.index + x.value;
  }

  TEST(st_wait_any_trivial)
  {
    auto result = coro::st::run(
      coro::deferred_co(async_both));
    
    // TODO: actual result 21
    ASSERT_EQ(0, result);
  }

  // TEST(st_yield_exception)
  // {
  //   ASSERT_THROW_WHAT(coro::st::run(coro::deferred_co([](coro::st::context & ctx) -> coro::co<void> {
  //     co_await coro::st::async_yield(ctx);
  //     throw std::runtime_error("Ups!");
  //     co_return;
  //   })), std::runtime_error, "Ups!");
  // }
} // anonymous namespace