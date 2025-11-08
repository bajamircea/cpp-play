#include "../test_lib/test.h"

#include "../coro_st_lib/wait_for.h"

#include "../coro_st_lib/coro_st.h"

#include "test_loop.h"

namespace
{
  static_assert(
    coro_st::is_co_task<
      coro_st::wait_for_task<
        coro_st::co<void>>>);
  static_assert(
    coro_st::is_co_task<
      coro_st::wait_for_task<
        coro_st::co<int>>>);

  TEST(wait_for_construction)
  {
    using namespace coro_st;
    using namespace coro_st::impl;

    coro_st_test::test_loop tl;

    auto child_task = async_yield();

    auto task = wait_for_task<decltype(child_task)>(
      child_task,
      std::chrono::steady_clock::now()
    );

    auto work = task.get_work();

    auto awaiter = work.get_awaiter(tl.ctx);
  }

  TEST(wait_for_void_has_value)
  {
    auto result = coro_st::run(coro_st::async_wait_for(
      coro_st::async_yield(),
      std::chrono::hours(24)
    )).value();
    ASSERT_TRUE(result.has_value());
  }

  TEST(wait_for_void_has_value_immediate)
  {
    auto result = coro_st::run(coro_st::async_wait_for(
      coro_st::async_noop(),
      std::chrono::hours(24)
    )).value();
    ASSERT_TRUE(result.has_value());
  }

  TEST(wait_for_void_nullopt)
  {
    auto result = coro_st::run(coro_st::async_wait_for(
      coro_st::async_suspend_forever(),
      std::chrono::seconds(0)
    )).value();
    ASSERT_FALSE(result.has_value());
  }

  coro_st::co<int> async_some_int()
  {
    co_await coro_st::async_yield();
    co_return 42;
  }

  TEST(wait_for_int_has_value)
  {
    auto result = coro_st::run(coro_st::async_wait_for(
      async_some_int(),
      std::chrono::hours(24)
    )).value();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(42, result.value());
  }

  TEST(wait_for_int_has_value_immediate)
  {
    auto result = coro_st::run(coro_st::async_wait_for(
      coro_st::async_just(42),
      std::chrono::hours(24)
    )).value();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(42, result.value());
  }

  coro_st::co<int> async_some_int_suspended()
  {
    co_await coro_st::async_suspend_forever();
    co_return 42;
  }

  TEST(wait_for_int_nullopt)
  {
    auto result = coro_st::run(coro_st::async_wait_for(
      async_some_int_suspended(),
      std::chrono::seconds(0)
    )).value();
    ASSERT_FALSE(result.has_value());
  }

  coro_st::co<std::optional<coro_st::void_result>> async_some_wait()
  {
    auto result = co_await coro_st::async_wait_for(
      coro_st::async_suspend_forever(),
      std::chrono::seconds(0)
    );
    co_return result;
  }

  TEST(wait_for_inside_co)
  {
    auto result = coro_st::run(async_some_wait()).value();
    ASSERT_FALSE(result.has_value());
  }

  TEST(wait_for_tree)
  {
    auto result = coro_st::run(coro_st::async_wait_for(
      coro_st::async_wait_for(
        std::invoke([]() -> coro_st::co<int> {
          co_await coro_st::async_suspend_forever();
          co_return 42;
        }),
        std::chrono::seconds(0)
      ),
      std::chrono::hours(24)
    )).value();
    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result.value().has_value());
  }

  TEST(wait_for_exception)
  {
    auto async_lambda = []() -> coro_st::co<void> {
      throw std::runtime_error("Ups!");
      co_return;
    };

    ASSERT_THROW_WHAT(coro_st::run(coro_st::async_wait_for(
      async_lambda(),
      std::chrono::hours(24)
    )), std::runtime_error, "Ups!");
  }

  TEST(wait_for_stopped)
  {
    auto result = coro_st::run(coro_st::async_wait_for(
      coro_st::async_just_stopped(),
      std::chrono::hours(24)));
    ASSERT_FALSE(result.has_value());
  }

  // coro_st::co<void> async_wait_for_does_not_compile()
  // {
  //   auto x = coro_st::async_wait_for(
  //     coro_st::async_suspend_forever(),
  //     std::chrono::seconds(0));
  //   // use of deleted function
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_wait_for_does_not_compile2()
  // {
  //   // ignoring return value
  //   coro_st::async_wait_for(
  //     coro_st::async_suspend_forever(),
  //     std::chrono::seconds(0));
  //   co_return;
  // }
} // anonymous namespace