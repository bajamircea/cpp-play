#include "../test_lib/test.h"

#include "../coro_st_lib/noop.h"

#include "../coro_st_lib/co.h"
#include "../coro_st_lib/coro_type_traits.h"
#include "../coro_st_lib/run.h"

#include "test_loop.h"

namespace
{
  static_assert(coro_st::is_co_task<coro_st::noop_task>);

  TEST(noop_chain_root_run)
  {
    coro_st::run(coro_st::async_noop());
  }

  TEST(noop_lambda_return_int)
  {
    auto async_lambda = []() -> coro_st::co<int> {
      co_await coro_st::async_noop();
      co_return 42;
    };
    int result = coro_st::run(async_lambda());

    ASSERT_EQ(42, result);
  }

  TEST(noop_chain_root)
  {
    coro_st_test::test_loop tl;

    auto task = coro_st::async_noop();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start_as_chain_root();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_TRUE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
  }

  TEST(noop_chain_root_cancellation)
  {
    coro_st_test::test_loop tl;

    auto task = coro_st::async_noop();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    // moved higher, if cancelled later
    // it's too late for async_noop
    tl.stop_source.request_stop();

    awaiter.start_as_chain_root();

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_FALSE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
    tl.run_pending_work();
    ASSERT_FALSE(tl.completed);
    ASSERT_TRUE(tl.cancelled);
  }

  TEST(noop_inside_co)
  {
    coro_st_test::test_loop tl;

    bool reached_noop{ false };

    auto async_lambda = [&reached_noop]() -> coro_st::co<void> {
      reached_noop = true;
      co_await coro_st::async_noop();
    };

    auto task = async_lambda();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start_as_chain_root();

    ASSERT_TRUE(reached_noop);

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_TRUE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
  }

  TEST(noop_inside_co_cancellation)
  {
    coro_st_test::test_loop tl;

    bool reached_noop{ false };

    auto async_lambda = [&reached_noop]() -> coro_st::co<void> {
      reached_noop = true;
      co_await coro_st::async_noop();
    };

    auto task = async_lambda();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    // moved higher, if cancelled later
    // it's too late for async_noop
    tl.stop_source.request_stop();

    awaiter.start_as_chain_root();

    ASSERT_TRUE(reached_noop);

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_FALSE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
    tl.run_pending_work();
    ASSERT_FALSE(tl.completed);
    ASSERT_TRUE(tl.cancelled);
  }

  // coro_st::co<void> async_noop_does_not_compile()
  // {
  //   auto x = coro_st::async_noop();
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_noop_does_not_compile2()
  // {
  //   coro_st::async_noop();
  //   co_return;
  // }
} // anonymous namespace