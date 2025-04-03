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
        wait_any_awaiter_chain_data_tuple_builder{shared_data, work} };
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

  // TEST(wait_any_inside_co)
  // {
  //   coro_st_test::test_loop tl;

  //   auto async_lambda = []() -> coro_st::co<void> {
  //     co_await coro_st::async_wait_any(
  //       coro_st::async_sleep_for(std::chrono::seconds(0)));
  //   };

  //   auto task = async_lambda();

  //   auto awaiter = task.get_work().get_awaiter(tl.ctx);

  //   awaiter.start_as_chain_root();

  //   // ASSERT_FALSE(tl.el.ready_queue_.empty());
  //   // ASSERT_TRUE(tl.el.timers_heap_.empty());

  //   // ASSERT_FALSE(tl.completed);
  //   // ASSERT_FALSE(tl.cancelled);
  //   // tl.run_pending_work();

  //   // ASSERT_FALSE(tl.completed);
  //   // ASSERT_TRUE(tl.cancelled);
  // }

  // void impl_construction(coro_st::context& parent_ctx,
  //   coro_st::sleep_task::work& co_work0,
  //   coro_st::sleep_task::work& co_work1)
  // {
  //   using SharedData = coro_st::impl::wait_any_awaiter_shared_data<void>;
  //   SharedData shared(parent_ctx);

  //   using ChainData =
  //     coro_st::impl::wait_any_awaiter_chain_data<
  //       SharedData, coro_st::sleep_task::work>;

  //   ChainData cd0(shared, co_work0);
  //   ChainData cd1(shared, co_work1);

  //   //std::tuple<ChainData> tcd{{shared, co_work0}};
  // }

  // TEST(wait_any_impl_construction)
  // {
  //   ASSERT_NE(nullptr, &impl_construction);
  // }

  TEST(wait_any_trivial)
  {
    // auto result = coro_st::run(coro_st::async_wait_any(
    //   coro_st::async_suspend_forever(),
    //   coro_st::async_sleep_for(std::chrono::seconds(0))
    // ));
    // ASSERT_EQ(1, result.index);
  }

//   TEST(yield_lambda_return_int)
//   {
//     auto async_lambda = []() -> coro_st::co<int> {
//       co_await coro_st::async_yield();
//       co_return 42;
//     };
//     int result = coro_st::run(async_lambda());

//     ASSERT_EQ(42, result);
//   }

//   // coro_st::co<void> async_yield_does_not_compile()
//   // {
//   //   auto x = coro_st::async_yield();
//   //   co_await std::move(x);
//   // }

//   // coro_st::co<void> async_yield_does_not_compile2()
//   // {
//   //   coro_st::async_yield();
//   //   co_return;
//   // }
} // anonymous namespace