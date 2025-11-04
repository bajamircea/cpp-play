#include "../test_lib/test.h"

#include "../coro_st_lib/yield.h"

#include "../coro_st_lib/co.h"
#include "../coro_st_lib/coro_type_traits.h"
#include "../coro_st_lib/run.h"

#include "test_loop.h"

namespace
{
  static_assert(coro_st::is_co_task<coro_st::yield_task>);

  TEST(yield_chain_root_run)
  {
    coro_st::run(coro_st::async_yield()).value();
  }

  TEST(yield_lambda_return_int)
  {
    auto async_lambda = []() -> coro_st::co<int> {
      co_await coro_st::async_yield();
      co_return 42;
    };
    int result = coro_st::run(async_lambda()).value();

    ASSERT_EQ(42, result);
  }

  TEST(yield_chain_root)
  {
    coro_st_test::test_loop tl;

    auto task = coro_st::async_yield();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start_as_chain_root();

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_FALSE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
    tl.run_pending_work();
    ASSERT_TRUE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
  }

  TEST(yield_chain_root_cancellation)
  {
    coro_st_test::test_loop tl;

    auto task = coro_st::async_yield();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    // moved higher, if cancelled later
    // it's too late for async_yield
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

  TEST(yield_inside_co)
  {
    coro_st_test::test_loop tl;

    bool reached_yield{ false };

    auto async_lambda = [&reached_yield]() -> coro_st::co<void> {
      reached_yield = true;
      co_await coro_st::async_yield();
    };

    auto task = async_lambda();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start_as_chain_root();

    ASSERT_TRUE(reached_yield);

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_FALSE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
    tl.run_pending_work();
    ASSERT_TRUE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
  }

  TEST(yield_inside_co_cancellation)
  {
    coro_st_test::test_loop tl;

    bool reached_yield{ false };

    auto async_lambda = [&reached_yield]() -> coro_st::co<void> {
      reached_yield = true;
      co_await coro_st::async_yield();
    };

    auto task = async_lambda();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    // moved higher, if cancelled later,
    // it's too late for async_yield
    tl.stop_source.request_stop();

    awaiter.start_as_chain_root();

    ASSERT_TRUE(reached_yield);

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_FALSE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
    tl.run_pending_work();
    ASSERT_FALSE(tl.completed);
    ASSERT_TRUE(tl.cancelled);
  }

  // coro_st::co<void> async_yield_does_not_compile()
  // {
  //   auto x = coro_st::async_yield();
  //   // use of deleted function
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_yield_does_not_compile2()
  // {
  //   // ignoring return value
  //   coro_st::async_yield();
  //   co_return;
  // }
} // anonymous namespace