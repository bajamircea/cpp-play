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

  coro::co<std::string> async_buzz()
  {
    auto f = coro::deferred_co(async_foo);
    co_await f();
    auto b = coro::deferred_co(async_bar, 42);
    auto x = co_await b();
    co_return x;
  }

  TEST(deferred_co_compiles)
  {
    ASSERT_NE(nullptr, &async_buzz);
  }
} // anonymous namespace