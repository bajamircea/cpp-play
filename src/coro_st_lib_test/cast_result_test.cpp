#include "../test_lib/test.h"

#include "../coro_st_lib/cast_result.h"

#include "../coro_st_lib/coro_st.h"

#include "test_loop.h"

#include <string>
#include <variant>

namespace
{
  static_assert(
    coro_st::is_co_task<
      coro_st::cast_result_task<
        void, coro_st::co<void>>>);
  static_assert(
    coro_st::is_co_task<
      coro_st::cast_result_task<
        int, coro_st::co<short>>>);
  static_assert(
    coro_st::is_co_task<
      coro_st::cast_result_task<
        void, coro_st::co<int>>>);

  TEST(cast_result_construction)
  {
    using namespace coro_st;
    using namespace coro_st::impl;

    coro_st_test::test_loop tl;

    auto child_task = async_yield();

    auto task = cast_result_task<void, decltype(child_task)>(
      child_task
    );

    auto work = task.get_work();

    [[maybe_unused]] auto awaiter = work.get_awaiter(tl.ctx);
  }

  TEST(cast_result_short_to_int)
  {
    auto result = coro_st::run(coro_st::async_cast_result<int>(
      std::invoke([]->coro_st::co<short> { co_return (short)42; })
    )).value();
    static_assert(std::is_same_v<int, decltype(result)>);
    ASSERT_EQ(42, result);
  }

  TEST(cast_result_int_to_void)
  {
    auto result = coro_st::run(coro_st::async_cast_result<void>(
      std::invoke([]->coro_st::co<int> { co_return 42; })
    )).value();
    static_assert(std::is_same_v<coro_st::void_result, decltype(result)>);
  }

  coro_st::co<int> async_some_int()
  {
    co_await coro_st::async_yield();
    co_return 42;
  }

  coro_st::co<const char*> async_some_c_str()
  {
    co_await coro_st::async_yield();
    co_return "42cstr";
  }

  coro_st::co<std::string> async_some_str()
  {
    co_await coro_st::async_yield();
    co_return "42str";
  }

  TEST(cast_result_wait_any_to_void)
  {
    auto result = coro_st::run(coro_st::async_wait_any(
      coro_st::async_cast_result<void>(async_some_c_str()),
      coro_st::async_cast_result<void>(async_some_int()),
      coro_st::async_cast_result<void>(async_some_str())
    )).value();
    ASSERT_EQ(0, result.index);
  }

  TEST(cast_result_wait_any_to_variant)
  {
    using common_type = std::variant<int, std::string>;
    auto result = coro_st::run(coro_st::async_wait_any(
      coro_st::async_cast_result<common_type>(async_some_c_str()),
      coro_st::async_cast_result<common_type>(async_some_int()),
      coro_st::async_cast_result<common_type>(async_some_str())
    )).value();
    static_assert(std::is_same_v<common_type, decltype(result.value)>);
    ASSERT_EQ(0, result.index);
    ASSERT_EQ(1, result.value.index());
    ASSERT_EQ("42cstr", std::get<std::string>(result.value));
  }

  TEST(cast_result_stopped)
  {
    auto run_result = coro_st::run(coro_st::async_cast_result<void>(
      coro_st::async_just_stopped()
    ));
    ASSERT_FALSE(run_result.has_value());
  }

  coro_st::co<int> async_some_cast()
  {
    auto result = co_await coro_st::async_cast_result<int>(
      coro_st::async_just((short)42)
    );
    static_assert(std::is_same_v<int, decltype(result)>);
    co_return result;
  }

  TEST(async_cast_result_inside_co)
  {
    auto result = coro_st::run(async_some_cast()).value();
    ASSERT_EQ(42, result);
  }

  // coro_st::co<void> async_cast_result_does_not_compile()
  // {
  //   auto x = coro_st::async_cast_result<int>(
  //     coro_st::async_just((short)42)
  //   );
  //   // use of deleted function
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_cast_result_does_not_compile2()
  // {
  //   // ignoring return value
  //   coro_st::async_cast_result<int>(
  //     coro_st::async_just((short)42)
  //   );
  //   co_return;
  // }
} // anonymous namespace