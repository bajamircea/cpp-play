#include "../test_lib/test.h"

#include "../coro_lib/deferred_member_co.h"

#include "../coro_lib/co.h"
#include "../coro_lib/deferred_co.h"

#include <string>

namespace
{
  struct Obj
  {
    int i_{};
    coro::co<void> async_foo(int i)
    {
      i_ = i;
      co_return;
    }

    [[nodiscard]] std::suspend_never async_awaitable() noexcept
    {
      return {};
    }

    [[nodiscard]] std::suspend_never async_awaitable2()
    {
      return {};
    }
  };

  coro::co<void> async_buzz()
  {
    Obj obj;
    auto m = coro::deferred_member_co(&Obj::async_foo, &obj, 42);
    co_await m();
    auto m2 = coro::deferred_member_co(&Obj::async_foo, &obj);
    co_await m2(42);
    static_assert(!std::is_nothrow_invocable_v<decltype(m2), int>);

    auto a = coro::deferred_member_co(&Obj::async_awaitable, &obj);
    co_await a();
    static_assert(std::is_nothrow_invocable_v<decltype(a)>);

    auto a2 = coro::deferred_member_co(&Obj::async_awaitable2, &obj);
    co_await a2();
    static_assert(!std::is_nothrow_invocable_v<decltype(a2)>);
  }

  TEST(deferred_member_co_compiles)
  {
    [[maybe_unused]] auto x = coro::deferred_co(async_buzz);
  }
} // anonymous namespace