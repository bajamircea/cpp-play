#include "../test_lib/test.h"

#include "../coro_st_lib/nursery.h"

#include "../coro_st_lib/co.h"
#include "../coro_st_lib/coro_type_traits.h"
#include "../coro_st_lib/just_stopped.h"
#include "../coro_st_lib/run.h"
#include "../coro_st_lib/suspend_forever.h"
#include "../coro_st_lib/yield.h"

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
    auto run_result = coro_st::run(
      n.async_run(
        coro_st::async_yield()
      ));
    ASSERT_TRUE(run_result.has_value());
  }

  TEST(nursery_lambda_and_stop)
  {
    int i = 0;
    coro_st::nursery n;
    auto async_initial_lambda = [](coro_st::nursery& n, int& i) -> coro_st::co<void> {
      co_await coro_st::async_yield();
      n.request_stop();
      i += 42;
      co_return;
    };
    auto run_result = coro_st::run(
      n.async_run(
        async_initial_lambda(n, i)
      ));
    ASSERT_TRUE(run_result.has_value());
    ASSERT_EQ(42, i);
  }

  TEST(nursery_lambda_by_capture_and_stop)
  {
    int i = 0;
    coro_st::nursery n;
    auto async_initial_lambda = [&n, &i]() -> coro_st::co<void> {
      co_await coro_st::async_yield();
      n.request_stop();
      i += 42;
      co_await coro_st::async_suspend_forever();
    };
    auto run_result = coro_st::run(
      n.async_run(
        async_initial_lambda()
      ));
    ASSERT_TRUE(run_result.has_value());
    ASSERT_EQ(42, i);
  }

  TEST(nursery_lambda_and_spawn)
  {
    int i = 0;
    coro_st::nursery n;
    auto async_initial_lambda = [&n, &i]() -> coro_st::co<void> {
      n.spawn_child([&i]() -> coro_st::co<void> {
        i += 2;
        co_return;
      });
      i += 40;
      co_return;
    };
    auto run_result = coro_st::run(
      n.async_run(
        async_initial_lambda()
      ));
    ASSERT_TRUE(run_result.has_value());
    ASSERT_EQ(42, i);
  }

  coro_st::co<void> async_some_nursery_child(int& i)
  {
    i += 1;
    co_return;
  }

  coro_st::co<void> async_some_nursery_initial(coro_st::nursery& n, int& i)
  {
    n.spawn_child(async_some_nursery_child, std::ref(i));
    n.spawn_child(async_some_nursery_child, std::ref(i));
    i += 40;
    co_return;
  }

  coro_st::co<int> async_some_nursery()
  {
    int i = 0;
    coro_st::nursery n;
    co_await n.async_run(
      async_some_nursery_initial(n, i));
    co_return i;
  }

  TEST(nursery_inside_co)
  {
    auto result = coro_st::run(async_some_nursery()).value();
    ASSERT_EQ(42, result);
  }

  TEST(nursery_exception_initial)
  {
    coro_st::nursery n;
    auto async_initial_lambda = [&n]() -> coro_st::co<void> {
      n.spawn_child([]() -> coro_st::co<void> {
        co_return;
      });
      throw std::runtime_error("Ups!");
      co_return;
    };
    ASSERT_THROW_WHAT(coro_st::run(
      n.async_run(
        async_initial_lambda()
      )), std::runtime_error, "Ups!");
  }

  TEST(nursery_exception_spawn)
  {
    coro_st::nursery n;
    auto async_initial_lambda = [&n]() -> coro_st::co<void> {
      n.spawn_child([]() -> coro_st::co<void> {
        throw std::runtime_error("Ups!");
        co_return;
      });
      co_return;
    };
    ASSERT_THROW_WHAT(coro_st::run(
      n.async_run(
        async_initial_lambda()
      )), std::runtime_error, "Ups!");
  }

  TEST(nursery_stopped_initial)
  {
    coro_st::nursery n;
    auto async_initial_lambda = [&n]() -> coro_st::co<void> {
      n.spawn_child([]() -> coro_st::co<void> {
        co_return;
      });
      co_await coro_st::async_just_stopped();
    };
    auto result = coro_st::run(
      n.async_run(
        async_initial_lambda()
      ));
    ASSERT_FALSE(result.has_value());
  }

  TEST(nursery_stopped_spawn)
  {
    coro_st::nursery n;
    auto async_initial_lambda = [&n]() -> coro_st::co<void> {
      n.spawn_child([]() -> coro_st::co<void> {
        co_await coro_st::async_just_stopped();
      });
      co_return;
    };
    auto result = coro_st::run(
      n.async_run(
        async_initial_lambda()
      ));
    ASSERT_FALSE(result.has_value());
  }

  // coro_st::co<void> async_nursery_does_not_compile()
  // {
  //   coro_st::nursery n;
  //   auto x = n.async_run(
  //     coro_st::async_yield()
  //   );
  //   // use of deleted function
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_nursery_does_not_compile2()
  // {
  //   coro_st::nursery n;
  //   // ignoring return value
  //   n.async_run(
  //     coro_st::async_yield()
  //   );
  //   co_return;
  // }

  // TEST(nursery_does_not_compile3)
  // {
  //   coro_st::nursery n;
  //   auto run_result = coro_st::run(
  //     n.async_run(
  //       []() -> coro_st::co<void> {
  //         co_return;
  //       } // lambda not invoked
  //     ));
  //   ASSERT_TRUE(run_result.has_value());
  // }
} // anonymous namespace