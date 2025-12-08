#include "../test_lib/test.h"

#include "deleter.h"

#include "../coro_st_lib/coro_st.h"

#include "test_loop.h"

#include <string>
#include <variant>

namespace
{
  static_assert(
    coro_st::is_co_task<
      coro_st_test::deleter_task<
        coro_st::co<void>>>);
  static_assert(
    coro_st::is_co_task<
      coro_st_test::deleter_task<
        coro_st::co<short>>>);
  static_assert(
    coro_st::is_co_task<
      coro_st_test::deleter_task<
        coro_st::co<int>>>);

  TEST(deleter_construction)
  {
    coro_st_test::test_loop tl;

    auto child_task = coro_st::async_just(42);

    auto task = coro_st_test::deleter_task<decltype(child_task)>(
      child_task
    );

    auto work = task.get_work();

    [[maybe_unused]] auto awaiter = work.get_awaiter(tl.ctx);
  }

  TEST(deleter_co_int)
  {
    auto result = coro_st::run(coro_st_test::async_deleter(
      std::invoke([]->coro_st::co<int> { co_return 42; })
    )).value();
    static_assert(std::is_same_v<int, decltype(result)>);
    ASSERT_EQ(42, result);
  }

  TEST(deleter_co_void)
  {
    auto run_result =coro_st::run(coro_st_test::async_deleter(
      std::invoke([]->coro_st::co<void> { co_return; })
    ));
    ASSERT_TRUE(run_result.has_value());
    static_assert(std::is_same_v<std::optional<coro_st::void_result>, decltype(run_result)>);
  }

  TEST(deleter_stopped)
  {
    auto run_result = coro_st::run(coro_st_test::async_deleter(
      coro_st::async_just_stopped()
    ));
    ASSERT_FALSE(run_result.has_value());
  }

  coro_st::co<int> async_some_deleter()
  {
    auto result = co_await coro_st_test::async_deleter(
      coro_st::async_just(42)
    );
    static_assert(std::is_same_v<int, decltype(result)>);
    co_return result;
  }

  TEST(deleter_inside_co)
  {
    auto result = coro_st::run(async_some_deleter()).value();
    ASSERT_EQ(42, result);
  }

  // coro_st::co<void> async_deleter_does_not_compile()
  // {
  //   auto x = coro_st_test::async_deleter(
  //     coro_st::async_just(42)
  //   );
  //   // use of deleted function
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_deleter_does_not_compile2()
  // {
  //   // ignoring return value
  //   coro_st_test::async_deleter(
  //     coro_st::async_just(42)
  //   );
  //   co_return;
  // }
} // anonymous namespace