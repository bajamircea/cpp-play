#include "../test_lib/test.h"

#include "../coro_lib/st_type_traits.h"

#include "../coro_lib/co.h"
#include "../coro_lib/deferred_co.h"

#include <string>

namespace
{
  coro::co<std::string> async_foo(coro::st::context&, const int&) {
    co_return "43";
  };

  TEST(co_compiles)
  {
    auto x = coro::deferred_co(async_foo, 0);
    static_assert(coro::st::is_context_callable_co<decltype(x)>);

    // TODO: add tests for sleep, yield etc.
  }
} // anonymous namespace