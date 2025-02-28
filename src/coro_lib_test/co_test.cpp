#include "../test_lib/test.h"

#include "../coro_lib/co.h"

#include <string>

namespace
{
  coro::co<void> foo()
  {
    co_return;
  }

  coro::co<std::string> bar()
  {
    co_return "abc";
  }

  coro::co<std::string> buzz()
  {
    co_await foo();
    auto x = co_await bar();
    co_return x;
  }

  // coro::co<void> does_not_compile()
  // {
  //   auto x = foo();
  //   co_await std::move(x);
  // }

  TEST(co_test_compiles)
  {
    ASSERT_NE(nullptr, &buzz);
  }
} // anonymous namespace