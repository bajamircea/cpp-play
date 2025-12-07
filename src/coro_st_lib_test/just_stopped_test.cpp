#include "../test_lib/test.h"

#include "../coro_st_lib/just_stopped.h"

#include "../coro_st_lib/co.h"
#include "../coro_st_lib/coro_type_traits.h"

#include "test_loop.h"

namespace
{
  static_assert(coro_st::is_co_task<coro_st::just_stopped_task>);

  TEST(just_stopped_chain_root)
  {
    coro_st_test::test_loop tl;

    auto task = coro_st::async_just_stopped();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    ASSERT_FALSE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);

    awaiter.start();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());
    ASSERT_FALSE(tl.result_ready);
    ASSERT_TRUE(tl.stopped);
  }

  TEST(just_stopped_inside_co)
  {
    coro_st_test::test_loop tl;

    auto async_lambda = []() -> coro_st::co<void> {
      co_await coro_st::async_just_stopped();
    };

    auto task = async_lambda();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_FALSE(tl.result_ready);
    ASSERT_TRUE(tl.stopped);
  }

  // coro_st::co<void> async_just_stopped_does_not_compile()
  // {
  //   auto x = coro_st::async_just_stopped();
  //   // use of deleted function
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_just_stopped_does_not_compile2()
  // {
  //   // ignoring return value
  //   coro_st::async_just_stopped();
  //   co_return;
  // }
} // anonymous namespace