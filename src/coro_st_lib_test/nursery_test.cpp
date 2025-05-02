#include "../test_lib/test.h"

#include "../coro_st_lib/nursery.h"

#include "../coro_st_lib/co.h"
#include "../coro_st_lib/yield.h"
#include "../coro_st_lib/coro_type_traits.h"
#include "../coro_st_lib/run.h"

#include "test_loop.h"

namespace
{
  static_assert(
    coro_st::is_co_task<
      coro_st::nursery::nursery_run_task<
        coro_st::co<void>>>);

  TEST(nursery_compiles)
  {
    coro_st_test::test_loop tl;

    coro_st::nursery n;
    auto async_initial_lambda = [](coro_st::nursery& n) -> coro_st::co<void> {
      co_await coro_st::async_yield();
      n.request_stop();
      co_return;
    };
    auto task = n.async_run(async_initial_lambda(n));
    auto awaiter = task.get_work().get_awaiter(tl.ctx);
  }

  TEST(nursery_trivial)
  {
    coro_st::nursery n;
    coro_st::run(
      n.async_run(
        coro_st::async_yield()
      ));
  }

  TEST(nursery_lambda_and_stop)
  {
    coro_st::nursery n;
    auto async_initial_lambda = [](coro_st::nursery& n) -> coro_st::co<void> {
      co_await coro_st::async_yield();
      n.request_stop();
      co_return;
    };
    coro_st::run(
      n.async_run(
        async_initial_lambda(n)
      ));
  }

  TEST(nursery_lambda_by_capture_and_stop)
  {
    coro_st::nursery n;
    auto async_initial_lambda = [&n]() -> coro_st::co<void> {
      co_await coro_st::async_yield();
      n.request_stop();
      co_return;
    };
    coro_st::run(
      n.async_run(
        async_initial_lambda()
      ));
  }

  TEST(nursery_lambda_and_spawn)
  {
    coro_st::nursery n;
    auto async_initial_lambda = [&n]() -> coro_st::co<void> {
      n.spawn_child(coro_st::async_yield);
      co_return;
    };
    coro_st::run(
      n.async_run(
        async_initial_lambda()
      ));
  }

  // TEST(yield_chain_root_run)
  // {
  //   coro_st::run(coro_st::async_yield());
  // }

  // TEST(yield_lambda_return_int)
  // {
  //   auto async_lambda = []() -> coro_st::co<int> {
  //     co_await coro_st::async_yield();
  //     co_return 42;
  //   };
  //   int result = coro_st::run(async_lambda());

  //   ASSERT_EQ(42, result);
  // }

  // TEST(yield_chain_root)
  // {
  //   coro_st_test::test_loop tl;

  //   auto task = coro_st::async_yield();

  //   auto awaiter = task.get_work().get_awaiter(tl.ctx);

  //   awaiter.start_as_chain_root();

  //   ASSERT_FALSE(tl.el.ready_queue_.empty());
  //   ASSERT_TRUE(tl.el.timers_heap_.empty());

  //   ASSERT_FALSE(tl.completed);
  //   ASSERT_FALSE(tl.cancelled);
  //   tl.run_pending_work();
  //   ASSERT_TRUE(tl.completed);
  //   ASSERT_FALSE(tl.cancelled);
  // }

  // TEST(yield_chain_root_cancellation)
  // {
  //   coro_st_test::test_loop tl;

  //   auto task = coro_st::async_yield();

  //   auto awaiter = task.get_work().get_awaiter(tl.ctx);

  //   // moved higher, if cancelled later
  //   // it's too late for async_yield
  //   tl.stop_source.request_stop();

  //   awaiter.start_as_chain_root();

  //   ASSERT_FALSE(tl.el.ready_queue_.empty());
  //   ASSERT_TRUE(tl.el.timers_heap_.empty());

  //   ASSERT_FALSE(tl.completed);
  //   ASSERT_FALSE(tl.cancelled);
  //   tl.run_pending_work();
  //   ASSERT_FALSE(tl.completed);
  //   ASSERT_TRUE(tl.cancelled);
  // }

  // TEST(yield_inside_co)
  // {
  //   coro_st_test::test_loop tl;

  //   bool reached_yield{ false };

  //   auto async_lambda = [&reached_yield]() -> coro_st::co<void> {
  //     reached_yield = true;
  //     co_await coro_st::async_yield();
  //   };

  //   auto task = async_lambda();

  //   auto awaiter = task.get_work().get_awaiter(tl.ctx);

  //   awaiter.start_as_chain_root();

  //   ASSERT_TRUE(reached_yield);

  //   ASSERT_FALSE(tl.el.ready_queue_.empty());
  //   ASSERT_TRUE(tl.el.timers_heap_.empty());

  //   ASSERT_FALSE(tl.completed);
  //   ASSERT_FALSE(tl.cancelled);
  //   tl.run_pending_work();
  //   ASSERT_TRUE(tl.completed);
  //   ASSERT_FALSE(tl.cancelled);
  // }

  // TEST(yield_inside_co_cancellation)
  // {
  //   coro_st_test::test_loop tl;

  //   bool reached_yield{ false };

  //   auto async_lambda = [&reached_yield]() -> coro_st::co<void> {
  //     reached_yield = true;
  //     co_await coro_st::async_yield();
  //   };

  //   auto task = async_lambda();

  //   auto awaiter = task.get_work().get_awaiter(tl.ctx);

  //   // moved higher, if cancelled later,
  //   // it's too late for async_yield
  //   tl.stop_source.request_stop();

  //   awaiter.start_as_chain_root();

  //   ASSERT_TRUE(reached_yield);

  //   ASSERT_FALSE(tl.el.ready_queue_.empty());
  //   ASSERT_TRUE(tl.el.timers_heap_.empty());

  //   ASSERT_FALSE(tl.completed);
  //   ASSERT_FALSE(tl.cancelled);
  //   tl.run_pending_work();
  //   ASSERT_FALSE(tl.completed);
  //   ASSERT_TRUE(tl.cancelled);
  // }

  // // coro_st::co<void> async_yield_does_not_compile()
  // // {
  // //   auto x = coro_st::async_yield();
  // //   co_await std::move(x);
  // // }

  // // coro_st::co<void> async_yield_does_not_compile2()
  // // {
  // //   coro_st::async_yield();
  // //   co_return;
  // // }
} // anonymous namespace