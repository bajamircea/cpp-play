#include "../test_lib/test.h"

#include "../coro_st_lib/event.h"

#include "../coro_st_lib/co.h"
#include "../coro_st_lib/coro_type_traits.h"
#include "../coro_st_lib/run.h"
#include "../coro_st_lib/wait_all.h"

#include "test_loop.h"

namespace
{
  static_assert(
    coro_st::is_co_task<
      coro_st::event::event_wait_task>);

  TEST(event_chain_root)
  {
    coro_st_test::test_loop tl;

    coro_st::event evt;

    ASSERT_FALSE(evt.notify_one());

    auto task = evt.async_wait();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_TRUE(evt.notify_one());

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_FALSE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
    tl.run_one_ready();
    ASSERT_TRUE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
  }

  TEST(event_chain_root_cancellation)
  {
    coro_st_test::test_loop tl;

    coro_st::event evt;

    auto task = evt.async_wait();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    tl.stop_source.request_stop();

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_FALSE(evt.notify_one());

    ASSERT_FALSE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
    tl.run_one_ready();
    ASSERT_FALSE(tl.result_ready);
    ASSERT_TRUE(tl.stopped);
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

    awaiter.start();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_EQ(1, evt.notify_all());

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_FALSE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
    tl.run_one_ready(2);
    ASSERT_TRUE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
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

    awaiter.start();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    tl.stop_source.request_stop();

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_EQ(0, evt.notify_all());

    ASSERT_FALSE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
    tl.run_one_ready();
    ASSERT_FALSE(tl.result_ready);
    ASSERT_TRUE(tl.stopped);
  }

  coro_st::co<void> async_event_notifier1(coro_st::event& evt)
  {
      evt.notify_all();
      co_return;
  }

  coro_st::co<void> async_event_waiter1(coro_st::event& evt)
  {
      co_await evt.async_wait();
  }

  TEST(event_usage1)
  {
      coro_st::event evt;
      auto result = coro_st::run(
          coro_st::async_wait_all(
              async_event_waiter1(evt),
              async_event_notifier1(evt)));
      ASSERT_TRUE(result.has_value());
  }

  struct event_usage_data2
  {
      int counter{ 0 };
      bool event_is_set{ false };
      coro_st::event evt;
  };

  coro_st::co<void> async_event_notifier2(event_usage_data2& data)
  {
      data.event_is_set = true;
      data.evt.notify_all();
      co_return;
  }

  coro_st::co<bool> async_event_waiter2(event_usage_data2& data)
  {
      if (data.event_is_set)
      {
          data.counter += 40;
          co_return false;
      }
      co_await data.evt.async_wait();
      data.counter += 2;
      co_return true;
  }

  TEST(event_usage2)
  {
      event_usage_data2 data;
      auto result = coro_st::run(
          coro_st::async_wait_all(
              async_event_waiter2(data),
              async_event_notifier2(data),
              async_event_waiter2(data))).value();
      ASSERT_TRUE(std::get<0>(result));
      ASSERT_FALSE(std::get<2>(result));
      ASSERT_EQ(42, data.counter);
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