#include "../test_lib/test.h"

#include "../coro_st_lib/stop_when.h"

#include "../coro_st_lib/coro_st.h"

#include "test_loop.h"

namespace
{
  static_assert(
    coro_st::is_co_task<
      coro_st::stop_when_task<
        coro_st::co<void>, coro_st::co<void>>>);
  static_assert(
    coro_st::is_co_task<
      coro_st::stop_when_task<
        coro_st::co<int>, coro_st::co<int>>>);

  TEST(stop_when_construction)
  {
    using namespace coro_st;
    using namespace coro_st::impl;

    coro_st_test::test_loop tl;

    auto child_task1 = async_yield();
    auto child_task2 = async_suspend_forever();

    auto task = coro_st::stop_when_task<decltype(child_task1), decltype(child_task2)>(
      child_task1,
      child_task2
    );

    auto work = task.get_work();

    auto awaiter = work.get_awaiter(tl.ctx);
  }

  TEST(stop_when_void_has_value)
  {
    auto result = coro_st::run(coro_st::async_stop_when(
      coro_st::async_yield(),
      coro_st::async_suspend_forever()
    )).value();
    ASSERT_TRUE(result.has_value());
  }

  TEST(stop_when_void_has_value_immediate)
  {
    auto result = coro_st::run(coro_st::async_stop_when(
      coro_st::async_noop(),
      coro_st::async_suspend_forever()
    )).value();
    ASSERT_TRUE(result.has_value());
  }

  TEST(stop_when_for_void_nullopt)
  {
    auto result = coro_st::run(coro_st::async_stop_when(
      coro_st::async_suspend_forever(),
      coro_st::async_noop()
    )).value();
    ASSERT_FALSE(result.has_value());
  }

  coro_st::co<int> async_some_int()
  {
    co_await coro_st::async_yield();
    co_return 42;
  }

  TEST(stop_when_int_has_value)
  {
    auto result = coro_st::run(coro_st::async_stop_when(
      async_some_int(),
      coro_st::async_suspend_forever()
    )).value();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(42, result.value());
  }

  TEST(stop_when_int_has_value_immediate)
  {
    auto result = coro_st::run(coro_st::async_stop_when(
      coro_st::async_just(42),
      coro_st::async_suspend_forever()
    )).value();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(42, result.value());
  }

  coro_st::co<int> async_some_int_suspended()
  {
    co_await coro_st::async_suspend_forever();
    co_return 42;
  }

  TEST(stop_when_int_nullopt)
  {
    auto result = coro_st::run(coro_st::async_stop_when(
      async_some_int_suspended(),
      coro_st::async_noop()
    )).value();
    ASSERT_FALSE(result.has_value());
  }

  coro_st::co<std::optional<coro_st::void_result>> async_some_wait()
  {
    auto result = co_await coro_st::async_stop_when(
      coro_st::async_suspend_forever(),
      coro_st::async_noop()
    );
    co_return result;
  }

  TEST(stop_when_inside_co)
  {
    auto result = coro_st::run(async_some_wait()).value();
    ASSERT_FALSE(result.has_value());
  }

  TEST(stop_when_tree)
  {
    auto result = coro_st::run(coro_st::async_stop_when(
      coro_st::async_stop_when(
        std::invoke([]() -> coro_st::co<int> {
          co_await coro_st::async_suspend_forever();
          co_return 42;
        }),
        coro_st::async_noop()
      ),
      coro_st::async_suspend_forever()
    )).value();
    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result.value().has_value());
  }

  TEST(stop_when_exception1)
  {
    auto async_lambda = []() -> coro_st::co<void> {
      throw std::runtime_error("Ups!");
      co_return;
    };

    ASSERT_THROW_WHAT(coro_st::run(coro_st::async_stop_when(
      async_lambda(),
      coro_st::async_suspend_forever()
    )), std::runtime_error, "Ups!");
  }

  TEST(stop_when_stopped1)
  {
    auto result = coro_st::run(coro_st::async_stop_when(
      coro_st::async_just_stopped(),
      coro_st::async_suspend_forever()));
    ASSERT_FALSE(result.has_value());
  }

  // For some reason that triggers what I believe to be a false positive
  // on g++ that made me use -Wno-dangling-pointer on g++ -O3 build
  TEST(stop_when_exception2)
  {
    auto async_lambda = []() -> coro_st::co<void> {
      throw std::runtime_error("Ups!");
      co_return;
    };

    ASSERT_THROW_WHAT(coro_st::run(coro_st::async_stop_when(
      coro_st::async_suspend_forever(),
      async_lambda()
    )), std::runtime_error, "Ups!");
  }

  TEST(stop_when_stopped2)
  {
    auto result = coro_st::run(coro_st::async_stop_when(
      coro_st::async_suspend_forever(),
      coro_st::async_just_stopped())).value();
    ASSERT_FALSE(result.has_value());
  }

  // coro_st::co<void> async_stop_when_does_not_compile()
  // {
  //   auto x = coro_st::async_stop_when(
  //     coro_st::async_suspend_forever(),
  //     coro_st::async_noop());
  //   // use of deleted function
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_stop_when_does_not_compile2()
  // {
  //   // ignoring return value
  //   coro_st::async_stop_when(
  //     coro_st::async_suspend_forever(),
  //     coro_st::async_noop());
  //   co_return;
  // }
} // anonymous namespace