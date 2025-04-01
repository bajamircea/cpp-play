#include "../test_lib/test.h"

#include "../coro_st_lib/sleep.h"

#include "../coro_st_lib/co.h"
#include "../coro_st_lib/coro_type_traits.h"
#include "../coro_st_lib/run.h"

namespace
{
  // TODO: same for others
  static_assert(coro_st::is_co_task<coro_st::sleep_task>);

  TEST(sleep_chain_root)
  {
    coro_st::run(coro_st::async_sleep_for(std::chrono::seconds(0)));
  }

  TEST(sleep_lambda_return_int)
  {
    auto async_lambda = []() -> coro_st::co<int> {
      co_await coro_st::async_sleep_for(std::chrono::seconds(0));
      co_return 42;
    };
    int result = coro_st::run(async_lambda());

    ASSERT_EQ(42, result);
  }

  // coro_st::co<void> async_sleep_does_not_compile()
  // {
  //   auto x = coro_st::async_sleep_for(std::chrono::seconds(0));
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_sleep_does_not_compile2()
  // {
  //   coro_st::async_sleep_for(std::chrono::seconds(0));
  //   co_return;
  // }
} // anonymous namespace