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

  coro::co<std::string> async_bar(const int& i)
  {
    co_return std::to_string(i);
  }

  [[nodiscard]] std::suspend_never async_awaitable() noexcept
  {
    return {};
  }

  [[nodiscard]] std::suspend_never async_awaitable2()
  {
    return {};
  }

  struct Obj
  {
    int i_{};
    coro::co<void> async_foo(int i)
    {
      i_ = i;
      co_return;
    }
  };

  coro::co<std::string> async_buzz()
  {
    auto f = coro::deferred_co(async_foo);
    co_await f();

    auto b = coro::deferred_co(async_bar, 4);
    auto x = co_await b();

    auto a = coro::deferred_co(async_awaitable);
    co_await a();
    static_assert(std::is_nothrow_invocable_v<decltype(a)>);

    auto a2 = coro::deferred_co(async_awaitable2);
    co_await a2();
    static_assert(!std::is_nothrow_invocable_v<decltype(a2)>);

    auto b1 = coro::deferred_co(async_bar);
    auto y = co_await b1(2);

    auto la = coro::deferred_co([]() -> coro::co<std::string> { co_return "la"; });
    auto z = co_await la();

    Obj obj;
    auto m = coro::deferred_co(&Obj::async_foo, &obj, 42);
    co_await m();
    auto ml = coro::deferred_co([&](int i) -> coro::co<void> {
      co_await obj.async_foo(i);
    });
    co_await ml(42);

    co_return x + y;
  }

  TEST(deferred_co_compiles)
  {
    [[maybe_unused]] auto x = coro::deferred_co(async_buzz);
  }
} // anonymous namespace