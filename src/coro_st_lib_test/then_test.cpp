#include "../test_lib/test.h"

#include "../coro_st_lib/then.h"

#include "../coro_st_lib/coro_st.h"

#include "test_loop.h"

#include <iostream>
#include <string>
#include <variant>
#include <type_traits>

namespace
{
  static_assert(
    coro_st::is_co_task<
      coro_st::then_task<
        coro_st::co<int>, int(*)(int)>>);
  static_assert(
    coro_st::is_co_task<
      coro_st::then_task<
        coro_st::co<void>, int (*)()>>);
  static_assert(
    coro_st::is_co_task<
      coro_st::then_task<
        coro_st::co<int>, void (*)(int)>>);

  TEST(then_construction)
  {
    using namespace coro_st;

    coro_st_test::test_loop tl;

    auto child_task = async_yield();

    auto lambda = [](){ return; };
    auto task = then_task(
      child_task, lambda
    );

    auto work = task.get_work();

    [[maybe_unused]] auto awaiter = work.get_awaiter(tl.ctx);
  }

  TEST(then_short_to_int)
  {
    auto result = coro_st::run(coro_st::async_then(
      coro_st::async_just((short)42),
      [](short x) -> int { return x; }
    )).value();
    static_assert(std::is_same_v<int, decltype(result)>);
    ASSERT_EQ(42, result);
  }

  TEST(then_int_to_void)
  {
    auto result = coro_st::run(coro_st::async_then(
      coro_st::async_just(42),
      [](int x) { if (x > 100) throw 42; }
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

  TEST(then_wait_any_to_variant)
  {
    using common_type = std::variant<int, std::string>;;
    auto result = coro_st::run(coro_st::async_wait_any(
      coro_st::async_then(async_some_c_str(),
        [](const char* x) -> common_type{ return x; }),
      coro_st::async_then(async_some_int(),
        [](int x) -> common_type{ return x; }),
      coro_st::async_then(async_some_str(),
        [](std::string x) -> common_type{ return x; })
    )).value();
    static_assert(std::is_same_v<common_type, decltype(result.value)>);
    ASSERT_EQ(0, result.index);
    ASSERT_EQ(1, result.value.index());
    ASSERT_EQ("42cstr", std::get<std::string>(result.value));
  }

  TEST(then_stopped)
  {
    auto run_result = coro_st::run(coro_st::async_then(
      coro_st::async_just_stopped(), [](){}
    ));
    ASSERT_FALSE(run_result.has_value());
  }

  coro_st::co<int> async_some_then()
  {
    auto result = co_await coro_st::async_then(
      coro_st::async_just(41), [](int x){ return ++x; }
    );
    static_assert(std::is_same_v<int, decltype(result)>);
    co_return result;
  }

  TEST(then_inside_co)
  {
    auto result = coro_st::run(async_some_then()).value();
    ASSERT_EQ(42, result);
  }

  coro_st::co<int> async_some_then_exception()
  {
    auto result = co_await coro_st::async_then(
      coro_st::async_just(41),
      [](int x){
        if (x < 100)
        {
          throw std::runtime_error("Ups!");
        }
        return x;
      }
    );
    static_assert(std::is_same_v<int, decltype(result)>);
    co_return result;
  }

  TEST(then_exception)
  {
    ASSERT_THROW_WHAT(coro_st::run(async_some_then_exception()),std::runtime_error, "Ups!");
  }

  // coro_st::co<void> async_then_does_not_compile()
  // {
  //   auto x = coro_st::async_then(
  //     coro_st::async_just(42),
  //     [](int x) { if (x > 100) throw 42; }
  //   );
  //   // use of deleted function
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_then_does_not_compile2()
  // {
  //   // ignoring return value
  //   coro_st::async_then(
  //     coro_st::async_just(42),
  //     [](int x) { if (x > 100) throw 42; }
  //   );
  //   co_return;
  // }
} // anonymous namespace