#include "../test_lib/test.h"

#include "../coro_lib/co.h"

#include <string>

namespace
{
  coro::co<void> async_foo()
  {
    co_return;
  }

  coro::co<std::string> async_bar()
  {
    co_return "abc";
  }

  coro::co<std::string> async_buzz()
  {
    co_await async_foo();
    auto x = co_await async_bar();
    co_return x;
  }

  // coro::co<void> async_does_not_compile()
  // {
  //   auto x = async_foo();
  //   co_await std::move(x);
  // }

  // coro::co<void> async_does_not_compile2()
  // {
  //   async_foo();
  //   co_return;
  // }

  TEST(co_compiles)
  {
    ASSERT_NE(nullptr, &async_buzz);
  }
} // anonymous namespace