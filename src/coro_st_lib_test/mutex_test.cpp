#include "../test_lib/test.h"

#include "../coro_st_lib/mutex.h"

#include "../coro_st_lib/co.h"
#include "../coro_st_lib/coro_type_traits.h"
#include "../coro_st_lib/wait_all.h"
#include "../coro_st_lib/yield.h"

#include "test_loop.h"

namespace
{
  static_assert(
    coro_st::is_co_task<
      coro_st::mutex::mutex_lock_task>);

  TEST(mutex_chain_root)
  {
    coro_st_test::test_loop tl;

    coro_st::mutex mtx;

    auto task = mtx.async_lock();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    ASSERT_FALSE(mtx.is_locked());
    awaiter.start();
    ASSERT_TRUE(mtx.is_locked());

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_TRUE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);

    {
      auto lock = awaiter.await_resume();

      ASSERT_TRUE(mtx.is_locked());

      ASSERT_TRUE(tl.el.ready_queue_.empty());
      ASSERT_TRUE(tl.el.timers_heap_.empty());
    }
    ASSERT_FALSE(mtx.is_locked());

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());
  }

  TEST(mutex_chain_root_locked)
  {
    coro_st_test::test_loop tl;

    coro_st::mutex mtx;

    auto task1 = mtx.async_lock();
    auto awaiter1 = task1.get_work().get_awaiter(tl.ctx);
    awaiter1.start();

    ASSERT_TRUE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
    tl.result_ready = false;

    auto task2 = mtx.async_lock();
    auto awaiter2 = task2.get_work().get_awaiter(tl.ctx);
    awaiter2.start();

    ASSERT_FALSE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    {
      auto lock = awaiter1.await_resume();

      ASSERT_TRUE(tl.el.ready_queue_.empty());
      ASSERT_TRUE(tl.el.timers_heap_.empty());

      ASSERT_FALSE(tl.result_ready);
      ASSERT_FALSE(tl.stopped);
    }

    ASSERT_TRUE(mtx.is_locked());

    ASSERT_FALSE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    tl.run_one_ready();
    ASSERT_TRUE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    {
      auto lock = awaiter2.await_resume();

      ASSERT_TRUE(mtx.is_locked());
    }
    ASSERT_FALSE(mtx.is_locked());

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());
  }

  TEST(mutex_chain_root_cancellation)
  {
    coro_st_test::test_loop tl;

    coro_st::mutex mtx;

    auto task = mtx.async_lock();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    tl.stop_source.request_stop();
    awaiter.start();

    ASSERT_FALSE(mtx.is_locked());

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_FALSE(tl.result_ready);
    ASSERT_TRUE(tl.stopped);
  }

  TEST(mutex_chain_root_cancellation_locked)
  {
    coro_st_test::test_loop tl;

    coro_st::mutex mtx;

    auto task1 = mtx.async_lock();
    auto awaiter1 = task1.get_work().get_awaiter(tl.ctx);
    awaiter1.start();

    ASSERT_TRUE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
    tl.result_ready = false;

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    auto task2 = mtx.async_lock();
    auto awaiter2 = task2.get_work().get_awaiter(tl.ctx);
    awaiter2.start();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    tl.stop_source.request_stop();

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    {
      auto lock = awaiter1.await_resume();

      ASSERT_TRUE(mtx.is_locked());
    }
    ASSERT_FALSE(mtx.is_locked());

    ASSERT_FALSE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
    tl.run_one_ready();
    ASSERT_FALSE(tl.result_ready);
    ASSERT_TRUE(tl.stopped);
  }

  TEST(mutex_inside_co)
  {
    coro_st_test::test_loop tl;

    coro_st::mutex mtx;

    auto async_lambda = [](coro_st::mutex& mtx) -> coro_st::co<void> {
      auto lock = co_await mtx.async_lock();
    };

    auto task = async_lambda(mtx);

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start();

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_FALSE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
    tl.run_one_ready();
    ASSERT_TRUE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
  }

  TEST(mutex_inside_co_locked)
  {
    coro_st_test::test_loop tl;

    coro_st::mutex mtx;

    auto async_lambda = [](coro_st::mutex& mtx) -> coro_st::co<void> {
      co_await coro_st::async_wait_all(
        std::invoke([](coro_st::mutex& mtx) -> coro_st::co<void>{
          ASSERT_FALSE(mtx.is_locked());
          auto lock = co_await mtx.async_lock();
          ASSERT_TRUE(mtx.is_locked());
          co_await coro_st::async_yield();
        }, mtx),
        std::invoke([](coro_st::mutex& mtx) -> coro_st::co<void>{
          ASSERT_TRUE(mtx.is_locked());
          auto lock = co_await mtx.async_lock();
          ASSERT_TRUE(mtx.is_locked());
        }, mtx)
      );
      auto lock = co_await mtx.async_lock();
    };

    auto task = async_lambda(mtx);

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start();

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_TRUE(mtx.is_locked());

    ASSERT_FALSE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
    tl.run_one_ready(5); // 5 = 3 coroutines, 1 yield, 1 mutex release
    ASSERT_TRUE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);

    ASSERT_FALSE(mtx.is_locked());
  }

  TEST(mutex_inside_co_cancellation)
  {
    coro_st_test::test_loop tl;

    coro_st::mutex mtx;

    auto async_lambda = [](coro_st::mutex& mtx) -> coro_st::co<void> {
      auto lock = co_await mtx.async_lock();
    };

    auto task = async_lambda(mtx);

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    tl.stop_source.request_stop();
    awaiter.start();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_FALSE(tl.result_ready);
    ASSERT_TRUE(tl.stopped);

    ASSERT_FALSE(mtx.is_locked());
  }

  TEST(mutex_inside_co_cancellation_locked)
  {
    coro_st_test::test_loop tl;

    coro_st::mutex mtx;

    auto async_lambda = [](coro_st::mutex& mtx) -> coro_st::co<void> {
      co_await coro_st::async_wait_all(
        std::invoke([](coro_st::mutex& mtx) -> coro_st::co<void>{
          ASSERT_FALSE(mtx.is_locked());
          auto lock = co_await mtx.async_lock();
          ASSERT_TRUE(mtx.is_locked());
          co_await coro_st::async_yield();
        }, mtx),
        std::invoke([](coro_st::mutex& mtx) -> coro_st::co<void>{
          ASSERT_TRUE(mtx.is_locked());
          auto lock = co_await mtx.async_lock();
          ASSERT_TRUE(mtx.is_locked());
        }, mtx)
      );
      auto lock = co_await mtx.async_lock();
    };

    auto task = async_lambda(mtx);

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start();

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_TRUE(mtx.is_locked());

    tl.stop_source.request_stop();

    ASSERT_FALSE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
    tl.run_one_ready(3);
    ASSERT_FALSE(tl.result_ready);
    ASSERT_TRUE(tl.stopped);

    ASSERT_FALSE(mtx.is_locked());
  }

  // coro_st::co<void> async_mutex_does_not_compile(coro_st::mutex& mtx)
  // {
  //   auto x = mtx.async_lock();
  //   // use of deleted function
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_mutex_does_not_compile2(coro_st::mutex& mtx)
  // {
  //   // ignoring return value
  //   mtx.async_lock();
  //   co_return;
  // }
} // anonymous namespace