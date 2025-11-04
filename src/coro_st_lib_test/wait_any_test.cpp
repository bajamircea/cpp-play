#include "../test_lib/test.h"

#include "../coro_st_lib/wait_any.h"

#include "../coro_st_lib/coro_st.h"

#include "test_loop.h"

namespace
{
  static_assert(
    coro_st::is_co_task<
      coro_st::wait_any_task<
        coro_st::co<void>,
        coro_st::sleep_task>>);

  TEST(wait_any_impl_construction)
  {
    using namespace coro_st;
    using namespace coro_st::impl;

    coro_st_test::test_loop tl;

    auto task = async_sleep_for(std::chrono::hours(24));

    auto work = task.get_work();

    wait_any_awaiter_shared_data<void> shared_data(tl.ctx);

    std::tuple<wait_any_awaiter_chain_data<decltype(shared_data), decltype(work)>>
      chain_datas{
        wait_any_awaiter_chain_data_tuple_builder{shared_data, 0, work} };
  }

  TEST(wait_any_construction)
  {
    using namespace coro_st;
    using namespace coro_st::impl;

    coro_st_test::test_loop tl;

    auto sleep_task = async_sleep_for(std::chrono::hours(24));

    auto task = wait_any_task<decltype(sleep_task)>(sleep_task);

    auto work = task.get_work();

    auto awaiter = work.get_awaiter(tl.ctx);
  }

  TEST(wait_any_trivial)
  {
    auto result = coro_st::run(coro_st::async_wait_any(
      coro_st::async_suspend_forever(),
      coro_st::async_sleep_for(std::chrono::seconds(0))
    )).value();
    ASSERT_EQ(1, result.index);
  }

  coro_st::co<size_t> async_some_wait()
  {
    auto result = co_await coro_st::async_wait_any(
      coro_st::async_suspend_forever(),
      coro_st::async_sleep_for(std::chrono::seconds(0))
    );
    co_return result.index;
  }

  TEST(wait_any_inside_co)
  {
    auto result = coro_st::run(async_some_wait()).value();
    ASSERT_EQ(1, result);
  }

  TEST(wait_any_tree)
  {
    auto result = coro_st::run(coro_st::async_wait_any(
      coro_st::async_suspend_forever(),
      coro_st::async_wait_any(
        std::invoke([]() -> coro_st::co<int> {
          co_return 42;
        }),
        coro_st::async_suspend_forever()
      )
    )).value();
    ASSERT_EQ(1, result.index);
    ASSERT_EQ(0, result.value.index);
    ASSERT_EQ(42, result.value.value);
  }

  TEST(wait_any_common_type)
  {
    auto result = coro_st::run(coro_st::async_wait_any(
        std::invoke([]() -> coro_st::co<short> {
          co_return (short)42;
        }),
        std::invoke([]() -> coro_st::co<int> {
          co_await coro_st::async_suspend_forever();
          co_return 1729;
        })
      )).value();
    ASSERT_EQ(0, result.index);
    ASSERT_EQ(42, result.value);
    static_assert(std::is_same_v<int, decltype(result.value)>);
  }

  TEST(wait_any_exception)
  {
    auto async_lambda = []() -> coro_st::co<void> {
      throw std::runtime_error("Ups!");
      co_return;
    };

    ASSERT_THROW_WHAT(coro_st::run(coro_st::async_wait_any(
      async_lambda(),
      coro_st::async_suspend_forever()
    )), std::runtime_error, "Ups!");
  }

  TEST(wait_any_stopped)
  {
    auto result = coro_st::run(coro_st::async_wait_any(
      coro_st::async_suspend_forever(),
      coro_st::async_just_stopped()));
    ASSERT_FALSE(result.has_value());
  }

  // coro_st::co<void> async_wait_any_does_not_compile()
  // {
  //   auto x = coro_st::async_wait_any(
  //     coro_st::async_suspend_forever(),
  //     coro_st::async_sleep_for(std::chrono::seconds(0))
  //   );
  //   // use of deleted function
  //   co_await std::move(x);
  // }
} // anonymous namespace