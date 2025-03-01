#include "../test_lib/test.h"

#include "../coro_lib/deferred_co.h"

#include "../coro_lib/co.h"

#include <string>

namespace
{
  coro::co<void> foo()
  {
    co_return;
  }

  coro::co<std::string> bar(const int & i)
  {
    co_return std::to_string(i);
  }

  coro::co<std::string> buzz()
  {
    auto f = coro::deferred_co(foo);
    co_await f();
    auto b = coro::deferred_co(bar, 42);
    auto x = co_await b();
    co_return x;
  }

  // coro::co<void> does_not_compile()
  // {
  //   auto x = foo();
  //   co_await std::move(x);
  // }

  TEST(deferred_co_compiles)
  {
    ASSERT_NE(nullptr, &buzz);
  }
} // anonymous namespace