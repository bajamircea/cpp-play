#include "../test_lib/test.h"

#include "../coro_lib/st_suspend_forever.h"

#include "../coro_lib/co.h"
#include "../coro_lib/st_run.h"
#include "../coro_lib/st_yield.h"
#include "../coro_lib/st_wait_any.h"

#include <stdexcept>
#include <string>

namespace
{
  coro::co<int> async_simple(coro::st::context & ctx)
  {
    auto x = co_await coro::st::async_wait_any(ctx,
      coro::st::async_suspend_forever,
      [](coro::st::context& ctx) -> coro::co<void>{
        co_await coro::st::async_suspend_forever(ctx);
        throw std::runtime_error("Ups!");
      },
      coro::st::async_yield);
    co_return x.index;
  }

  TEST(st_suspend_forever_simple)
  {
    auto result = coro::st::run(async_simple);
    
    ASSERT_EQ(2, result);
  }

  // coro::co<void> async_suspend_forever_does_not_compile(coro::st::context & ctx)
  // {
  //   auto x = coro::st::async_suspend_forever(ctx);
  //   co_await std::move(x);
  // }

  // coro::co<void> async_suspend_forever_does_not_compile2(coro::st::context & ctx)
  // {
  //   coro::st::async_suspend_forever(ctx);
  //   co_return;
  // }
} // anonymous namespace