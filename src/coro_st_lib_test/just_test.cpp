#include "../test_lib/test.h"

#include "../coro_st_lib/just.h"

#include "../coro_st_lib/co.h"
#include "../coro_st_lib/coro_type_traits.h"

#include "test_loop.h"

namespace
{
  static_assert(coro_st::is_co_task<coro_st::just_task<int>>);

  TEST(just_chain_root)
  {
    coro_st_test::test_loop tl;

    auto task = coro_st::async_just(42);

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    ASSERT_FALSE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);

    awaiter.start();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());
    ASSERT_TRUE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);

    ASSERT_FALSE(awaiter.get_result_exception());
    ASSERT_EQ(42, awaiter.await_resume());
  }

  TEST(just_inside_co)
  {
    coro_st_test::test_loop tl;

    auto async_lambda = []() -> coro_st::co<int> {
      co_return co_await coro_st::async_just(42);
    };

    auto task = async_lambda();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    ASSERT_FALSE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
    awaiter.start();
    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());
    tl.run_pending_work();
    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());
    ASSERT_TRUE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
    ASSERT_FALSE(awaiter.get_result_exception());
    ASSERT_EQ(42, awaiter.await_resume());
  }

  // coro_st::co<void> async_just_does_not_compile()
  // {
  //   auto x = coro_st::async_just(42);
  //   // use of deleted function
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_just_does_not_compile2()
  // {
  //   // ignoring return value
  //   coro_st::async_just();
  //   co_return;
  // }

  // coro_st::co<void> async_just_does_not_compile3()
  // {
  //   struct X
  //   {
  //     X() noexcept = default;
  //     X(const X&) noexcept = default;
  //     X(X&&) noexcept = delete;
  //   };

  //   // no matching function for call to 'async_just'
  //   co_await coro_st::async_just(X());
  // }
} // anonymous namespace