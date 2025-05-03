#include "../test_lib/test.h"

#include "../coro_st_lib/event.h"

#include "../coro_st_lib/co.h"
#include "../coro_st_lib/coro_type_traits.h"
#include "../coro_st_lib/run.h"

#include "test_loop.h"

namespace
{
  static_assert(
    coro_st::is_co_task<
      coro_st::event::event_wait_task>);

  // TEST(sleep_chain_root_run)
  // {
  //   coro_st::run(coro_st::async_sleep_for(std::chrono::seconds(0)));
  // }

  // TEST(sleep_lambda_return_int)
  // {
  //   auto async_lambda = []() -> coro_st::co<int> {
  //     co_await coro_st::async_sleep_for(std::chrono::seconds(0));
  //     co_return 42;
  //   };
  //   int result = coro_st::run(async_lambda());

  //   ASSERT_EQ(42, result);
  // }

  TEST(event_chain_root)
  {
    coro_st_test::test_loop tl;

    coro_st::event evt;

    ASSERT_FALSE(evt.notify_one());

    auto task = evt.async_wait();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start_as_chain_root();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_TRUE(evt.notify_one());

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_FALSE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
    tl.run_pending_work();
    ASSERT_TRUE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
  }

  TEST(event_chain_root_cancellation)
  {
    coro_st_test::test_loop tl;

    coro_st::event evt;

    auto task = evt.async_wait();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start_as_chain_root();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    tl.stop_source.request_stop();

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_FALSE(evt.notify_one());

    ASSERT_FALSE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
    tl.run_pending_work();
    ASSERT_FALSE(tl.completed);
    ASSERT_TRUE(tl.cancelled);
  }

  TEST(event_inside_co)
  {
    coro_st_test::test_loop tl;

    coro_st::event evt;

    auto async_lambda = [](coro_st::event& evt) -> coro_st::co<void> {
      co_await evt.async_wait();
    };

    auto task = async_lambda(evt);

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start_as_chain_root();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_EQ(1, evt.notify_all());

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_FALSE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
    tl.run_pending_work();
    ASSERT_TRUE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
  }

  TEST(event_inside_co_cancellation)
  {
    coro_st_test::test_loop tl;

    coro_st::event evt;

    auto async_lambda = [](coro_st::event& evt) -> coro_st::co<void> {
      co_await evt.async_wait();
    };

    auto task = async_lambda(evt);

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start_as_chain_root();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    tl.stop_source.request_stop();

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_EQ(0, evt.notify_all());

    ASSERT_FALSE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
    tl.run_pending_work();
    ASSERT_FALSE(tl.completed);
    ASSERT_TRUE(tl.cancelled);
  }

  // coro_st::co<void> async_event_does_not_compile(coro_st::event& evt)
  // {
  //   auto x = evt.async_wait();
  //   // use of deleted function
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_event_does_not_compile2(coro_st::event& evt)
  // {
  //   // ignoring return value
  //   evt.async_wait();
  //   co_return;
  // }
} // anonymous namespace