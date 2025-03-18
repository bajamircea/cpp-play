#include "../test_lib/test.h"

#include "../coro_lib/deferred_type_traits.h"

#include "../coro_lib/co.h"
#include "../coro_lib/deferred_co.h"

#include <string>

namespace
{
  coro::co<std::string> async_foo(const int&) {
    co_return "42";
  };

  TEST(co_compiles)
  {
    static_assert(!coro::is_deferred_co<std::string>);

    // TODO(low pri): fix: it does not compile, should compile and not be a deferred_co
    //auto la = []() -> void{};
    //static_assert(!coro::is_deferred_co<decltype(la)>);

    auto x = coro::deferred_co(async_foo, 0);
    static_assert(coro::is_deferred_co<decltype(x)>);
    static_assert(coro::deferred_co_has_non_member_operator_co_await<decltype(x)>);
    static_assert(std::is_same_v<std::string, coro::deferred_co_return_type<decltype(x)>>);

    auto y = coro::deferred_co([]() -> std::suspend_always {
      return {};
    });
    static_assert(coro::is_deferred_co<decltype(y)>);
    static_assert(!coro::deferred_co_has_non_member_operator_co_await<decltype(y)>);
    // TODO(low pri): fix ability to get the return type of awaitables that do not provide co_return_type
    //static_assert(std::is_same_v<void, coro::deferred_context_co_return_type<decltype(y)>>);
  }
} // anonymous namespace