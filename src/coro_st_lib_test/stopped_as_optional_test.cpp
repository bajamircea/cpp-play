#include "../test_lib/test.h"

#include "../coro_st_lib/stopped_as_optional.h"

#include "../coro_st_lib/coro_st.h"

#include "test_loop.h"

namespace
{
  static_assert(
    coro_st::is_co_task<
      coro_st::stopped_as_optional_task<
        coro_st::co<void>>>);
  static_assert(
    coro_st::is_co_task<
      coro_st::stopped_as_optional_task<
        coro_st::co<int>>>);

  TEST(stopped_as_optional_construction)
  {
    using namespace coro_st;
    using namespace coro_st::impl;

    coro_st_test::test_loop tl;

    auto child_task = async_yield();

    auto task = stopped_as_optional_task<decltype(child_task)>(
      child_task
    );

    auto work = task.get_work();

    [[maybe_unused]] auto awaiter = work.get_awaiter(tl.ctx);
  }

  TEST(stopped_as_optional_has_value)
  {
    auto result = coro_st::run(coro_st::async_stopped_as_optional(
      coro_st::async_yield()
    )).value();
    ASSERT_TRUE(result.has_value());
  }

  TEST(stopped_as_optional_immediate)
  {
    auto result = coro_st::run(coro_st::async_stopped_as_optional(
      coro_st::async_noop()
    )).value();
    ASSERT_TRUE(result.has_value());
  }

  TEST(stopped_as_optional_nullopt)
  {
    auto result = coro_st::run(coro_st::async_stopped_as_optional(
      coro_st::async_just_stopped()
    )).value();
    ASSERT_FALSE(result.has_value());
  }

  coro_st::co<int> async_some_int()
  {
    co_await coro_st::async_yield();
    co_return 42;
  }

  TEST(stopped_as_optional_int_has_value)
  {
    auto result = coro_st::run(coro_st::async_stopped_as_optional(
      async_some_int()
    )).value();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(42, result.value());
  }

  coro_st::co<int> async_some_int_immediate()
  {
    co_return 42;
  }

  TEST(stopped_as_optional_int_has_value_immediate)
  {
    auto result = coro_st::run(coro_st::async_stopped_as_optional(
      async_some_int_immediate()
    )).value();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(42, result.value());
  }

  coro_st::co<int> async_some_int_stopped()
  {
    co_await coro_st::async_just_stopped();
    co_return 42;
  }

  TEST(stopped_as_optional_int_nullopt)
  {
    auto result = coro_st::run(coro_st::async_stopped_as_optional(
      async_some_int_stopped()
    )).value();
    ASSERT_FALSE(result.has_value());
  }

  coro_st::co<std::optional<coro_st::void_result>> async_some_stopped()
  {
    auto result = co_await coro_st::async_stopped_as_optional(
      coro_st::async_just_stopped()
    );
    co_return result;
  }

  TEST(stopped_as_optional_inside_co)
  {
    auto result = coro_st::run(async_some_stopped()).value();
    ASSERT_FALSE(result.has_value());
  }

  TEST(stopped_as_optional_tree)
  {
    auto result = coro_st::run(coro_st::async_stopped_as_optional(
      coro_st::async_wait_all(
        coro_st::async_yield(),
        coro_st::async_just_stopped(),
        coro_st::async_yield()
    ))).value();
    ASSERT_FALSE(result.has_value());
  }

  TEST(stopped_as_optional_exception)
  {
    auto async_lambda = []() -> coro_st::co<void> {
      throw std::runtime_error("Ups!");
      co_return;
    };

    ASSERT_THROW_WHAT(coro_st::run(coro_st::async_stopped_as_optional(
      async_lambda()
    )), std::runtime_error, "Ups!");
  }

  // coro_st::co<void> async_stopped_as_optional_does_not_compile()
  // {
  //   auto x = coro_st::async_stopped_as_optional(
  //     coro_st::async_suspend_forever()
  //   );
  //   // use of deleted function
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_stopped_as_optional_does_not_compile2()
  // {
  //   // ignoring return value
  //   coro_st::async_stopped_as_optional(
  //     coro_st::async_suspend_forever()
  //   );
  //   co_return;
  // }
} // anonymous namespace