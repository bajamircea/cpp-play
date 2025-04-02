#include "../test_lib/test.h"

#include "../coro_st_lib/suspend_forever.h"

#include "../coro_st_lib/co.h"
#include "../coro_st_lib/coro_type_traits.h"

#include "test_loop.h"

namespace
{
  static_assert(coro_st::is_co_task<coro_st::suspend_forever_task>);

  TEST(suspend_forever_chain_root)
  {
    coro_st_test::test_loop tl;

    auto task = coro_st::async_suspend_forever();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start_as_chain_root();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    tl.stop_source.request_stop();

    ASSERT_FALSE(tl.el.ready_queue_.empty());

    ASSERT_FALSE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
    tl.run_pending_work();
    ASSERT_FALSE(tl.completed);
    ASSERT_TRUE(tl.cancelled);
  }

  TEST(suspend_forever_inside_co)
  {
    coro_st_test::test_loop tl;

    auto async_lambda = []() -> coro_st::co<void> {
      co_await coro_st::async_suspend_forever();
    };

    auto task = async_lambda();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start_as_chain_root();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    tl.stop_source.request_stop();

    ASSERT_FALSE(tl.el.ready_queue_.empty());

    ASSERT_FALSE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
    tl.run_pending_work();

    ASSERT_FALSE(tl.completed);
    ASSERT_TRUE(tl.cancelled);
  }

  // coro_st::co<void> async_suspend_forever_does_not_compile()
  // {
  //   auto x = coro_st::async_suspend_forever();
  //   // use of deletef function
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_suspend_forever_does_not_compile2()
  // {
  //   // ignoring return value
  //   coro_st::async_suspend_forever();
  //   co_return;
  // }
} // anonymous namespace