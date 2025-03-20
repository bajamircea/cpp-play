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

  coro::co<void> async_dangerous()
  {
    // Ideally should not compile as the non member
    // `operator co_await` is private, but
    // unfortunately it does compile
    auto x = operator co_await(async_foo());
    co_await x;
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

  // coro::co<void> async_does_not_compile3()
  // {
  //   auto x = async_foo();
  //   co_await operator co_await(x);
  // }

  TEST(co_compiles)
  {
    ASSERT_NE(nullptr, &async_buzz);
    ASSERT_NE(nullptr, &async_dangerous);
  }
} // anonymous namespace