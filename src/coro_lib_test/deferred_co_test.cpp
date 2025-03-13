#include "../test_lib/test.h"

#include "../coro_lib/deferred_co.h"

#include "../coro_lib/co.h"

#include <string>

namespace
{
  coro::co<void> async_foo()
  {
    co_return;
  }

  coro::co<std::string> async_bar(const int & i)
  {
    co_return std::to_string(i);
  }

  [[nodiscard]] std::suspend_never async_awaitable() noexcept
  {
    return {};
  }

  coro::co<std::string> async_buzz()
  {
    auto f = coro::deferred_co(async_foo);
    co_await f();
    
    auto b = coro::deferred_co(async_bar, 4);
    auto x = co_await b();

    auto a = coro::deferred_co(async_awaitable);
    co_await a();

    auto b1 = coro::deferred_co(async_bar);
    auto y = co_await b1(2);

    auto la = coro::deferred_co([]() -> coro::co<std::string> { co_return "la"; });
    auto z = co_await la();

    co_return x + y;
  }

  TEST(deferred_co_compiles)
  {
    ASSERT_NE(nullptr, &async_buzz);
  }
} // anonymous namespace