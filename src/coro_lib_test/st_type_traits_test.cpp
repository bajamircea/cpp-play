#include "../test_lib/test.h"

#include "../coro_lib/st_type_traits.h"

#include "../coro_lib/co.h"
#include "../coro_lib/deferred_co.h"

#include <string>
#include <iostream>

namespace
{
  coro::co<std::string> async_foo(coro::st::context& ctx, const int&) {
    co_return "43";
  };

  TEST(co_compiles)
  {
    auto x = coro::deferred_co<std::string>(async_foo, 0);
    static_assert(coro::st::deferred_context_co<decltype(x)>);
    static_assert(coro::st::deferred_context_co_has_non_member_operator_co_await<decltype(x)>);
    static_assert(std::is_same_v<std::string, coro::st::deferred_context_co_return_type<decltype(x)>>);
  }
} // anonymous namespace