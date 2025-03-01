#include "../test_lib/test.h"

#include "../coro_lib/st.h"

#include <string>

namespace
{
  TEST(st_trivial)
  {
    coro::st::context ctx;

    ctx.run(coro::deferred_co([]() -> coro::co<void> {
      co_return;
    }));
  }

  TEST(st_return)
  {
    coro::st::context ctx;

    int result = ctx.run(coro::deferred_co([]() -> coro::co<int> {
      co_return 42;
    }));

    ASSERT_EQ(42, result);
  }
} // anonymous namespace