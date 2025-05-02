#include "../test_lib/test.h"

#include "../coro_st_lib/wait_all.h"

#include "../coro_st_lib/coro_st.h"

#include "test_loop.h"

namespace
{
  static_assert(
    coro_st::is_co_task<
      coro_st::wait_all_task<
        coro_st::co<void>,
        coro_st::sleep_task>>);

  TEST(wait_all_impl_construction)
  {
    using namespace coro_st;
    using namespace coro_st::impl;

    coro_st_test::test_loop tl;

    auto task = async_sleep_for(std::chrono::hours(24));

    auto work = task.get_work();

    wait_all_awaiter_shared_data shared_data(tl.ctx);

    std::tuple<wait_all_awaiter_chain_data<decltype(work)>>
      chain_datas{
        wait_all_awaiter_chain_data_tuple_builder{shared_data, work} };
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

  TEST(wait_all_trivial)
  {
    coro_st::run(coro_st::async_wait_any(
      coro_st::async_yield(),
      coro_st::async_sleep_for(std::chrono::seconds(0))
    ));
  }

  coro_st::co<int> async_some_wait()
  {
    auto result = co_await coro_st::async_wait_all(
      coro_st::async_sleep_for(std::chrono::seconds(0)),
      std::invoke([]() -> coro_st::co<int> {
        co_return 42;
      })
    );
    static_assert(std::is_same_v<coro_st::void_result&, decltype(std::get<0>(result))>);
    static_assert(std::is_same_v<int&, decltype(std::get<1>(result))>);
    co_return std::get<1>(result);
  }

  TEST(wait_all_inside_co)
  {
    auto result = coro_st::run(async_some_wait());
    ASSERT_EQ(42, result);
  }

  TEST(wait_all_tree)
  {
    auto result = coro_st::run(coro_st::async_wait_any(
      coro_st::async_suspend_forever(),
      coro_st::async_wait_all(
        std::invoke([]() -> coro_st::co<int> {
          co_return 42;
        }),
        coro_st::async_yield()
      )
    ));
    ASSERT_EQ(1, result.index);
    ASSERT_EQ(42, std::get<0>(result.value));
  }

  TEST(wait_all_exception)
  {
    auto async_lambda = []() -> coro_st::co<void> {
      throw std::runtime_error("Ups!");
      co_return;
    };

    ASSERT_THROW_WHAT(coro_st::run(coro_st::async_wait_all(
      async_lambda(),
      coro_st::async_suspend_forever()
    )), std::runtime_error, "Ups!");
  }

  // coro_st::co<void> async_wait_any_does_not_compile()
  // {
  //   auto x = coro_st::async_wait_all(
  //     coro_st::async_yield(),
  //     coro_st::async_sleep_for(std::chrono::seconds(0))
  //   );
  //   // use of deleted function
  //   co_await std::move(x);
  // }
} // anonymous namespace