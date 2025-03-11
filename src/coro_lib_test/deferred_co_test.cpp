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

  [[nodiscard]] std::suspend_never async_test() noexcept
  {
    return {};
  }

  coro::co<std::string> async_buzz()
  {
    auto f = coro::deferred_co<void>(async_foo);
    co_await f();
    
    auto b = coro::deferred_co<std::string>(async_bar, 4);
    auto x = co_await b();

    auto a = coro::deferred_co<void>(async_test);
    co_await a();

    auto b1 = coro::deferred_co<std::string>(async_bar);
    auto y = co_await b1(2);

    co_return x + y;
  }

  TEST(deferred_co_compiles)
  {
    ASSERT_NE(nullptr, &async_buzz);
  }
} // anonymous namespace