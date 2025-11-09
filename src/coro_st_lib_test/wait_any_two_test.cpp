#include "../test_lib/test.h"

#include "../coro_st_lib/wait_any_two.h"

#include "../coro_st_lib/coro_st.h"

#include "test_loop.h"

namespace
{
  static_assert(
    coro_st::is_co_task<
      coro_st::wait_any_two_task<
        coro_st::co<void>, coro_st::co<void>>>);
  static_assert(
    coro_st::is_co_task<
      coro_st::wait_any_two_task<
        coro_st::co<int>, coro_st::co<int>>>);

  TEST(wait_any_two_construction)
  {
    using namespace coro_st;
    using namespace coro_st::impl;

    coro_st_test::test_loop tl;

    auto child_task1 = async_yield();
    auto child_task2 = async_suspend_forever();

    auto task = coro_st::wait_any_two_task<decltype(child_task1), decltype(child_task2)>(
      child_task1,
      child_task2
    );

    auto work = task.get_work();

    auto awaiter = work.get_awaiter(tl.ctx);
  }

  TEST(wait_any_two_void_completes1)
  {
    auto run_result = coro_st::run(coro_st::async_wait_any_two(
      coro_st::async_yield(),
      coro_st::async_suspend_forever()
    ));
    ASSERT_TRUE(run_result.has_value());
  }

  TEST(wait_any_two_void_completes2)
  {
    auto run_result = coro_st::run(coro_st::async_wait_any_two(
      coro_st::async_suspend_forever(),
      coro_st::async_yield()
    ));
    ASSERT_TRUE(run_result.has_value());
  }

  TEST(wait_any_two_void_completes1_immediate)
  {
    auto run_result = coro_st::run(coro_st::async_wait_any_two(
      coro_st::async_noop(),
      coro_st::async_suspend_forever()
    ));
    ASSERT_TRUE(run_result.has_value());
  }

  TEST(wait_any_two_void_completes2_immediate)
  {
    auto run_result = coro_st::run(coro_st::async_wait_any_two(
      coro_st::async_suspend_forever(),
      coro_st::async_noop()
    ));
    ASSERT_TRUE(run_result.has_value());
  }

  TEST(wait_any_two_void_completes_both_immediate)
  {
    auto run_result = coro_st::run(coro_st::async_wait_any_two(
      coro_st::async_noop(),
      coro_st::async_noop()
    ));
    ASSERT_TRUE(run_result.has_value());
  }

  coro_st::co<int> async_some_other_int()
  {
    co_await coro_st::async_yield();
    co_return 43;
  }

  coro_st::co<int> async_some_int()
  {
    co_return 42;
  }

  TEST(wait_any_two_int_has_value)
  {
    auto result = coro_st::run(coro_st::async_wait_any_two(
      async_some_other_int(),
      async_some_int()
    )).value();
    ASSERT_EQ(42, result);
  }

  TEST(wait_any_two_stop)
  {
    auto run_result = coro_st::run(coro_st::async_wait_any_two(
      coro_st::async_suspend_forever(),
      coro_st::async_just_stopped()
    ));
    ASSERT_FALSE(run_result.has_value());
  }

  coro_st::co<void> async_some_wait()
  {
    co_await coro_st::async_wait_any_two(
      coro_st::async_suspend_forever(),
      coro_st::async_noop()
    );
    co_return;
  }

  TEST(wait_any_two_inside_co)
  {
    auto run_result = coro_st::run(async_some_wait());
    ASSERT_TRUE(run_result.has_value());
  }

  TEST(wait_any_two_tree_and_parent_stopped)
  {
    auto run_result = coro_st::run(coro_st::async_wait_any_two(
      coro_st::async_wait_any_two(
        coro_st::async_suspend_forever(),
        coro_st::async_noop()
      ),
      coro_st::async_just_stopped()
    ));
    ASSERT_FALSE(run_result.has_value());
  }

  TEST(wait_any_two_when_exception1)
  {
    auto async_lambda = []() -> coro_st::co<void> {
      throw std::runtime_error("Ups!");
      co_return;
    };

    ASSERT_THROW_WHAT(coro_st::run(coro_st::async_wait_any_two(
      async_lambda(),
      coro_st::async_suspend_forever()
    )), std::runtime_error, "Ups!");
  }

  TEST(wait_any_two_when_exception2)
  {
    auto async_lambda = []() -> coro_st::co<void> {
      throw std::runtime_error("Ups!");
      co_return;
    };

    ASSERT_THROW_WHAT(coro_st::run(coro_st::async_wait_any_two(
      coro_st::async_suspend_forever(),
      async_lambda()
    )), std::runtime_error, "Ups!");
  }

  // coro_st::co<void> async_wait_any_two_does_not_compile()
  // {
  //   auto x = coro_st::async_wait_any_two(
  //     coro_st::async_suspend_forever(),
  //     coro_st::async_noop());
  //   // use of deleted function
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_wait_any_two_does_not_compile2()
  // {
  //   // ignoring return value
  //   coro_st::async_wait_any_two(
  //     coro_st::async_suspend_forever(),
  //     coro_st::async_noop());
  //   co_return;
  // }
} // anonymous namespace