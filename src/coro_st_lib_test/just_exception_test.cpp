#include "../test_lib/test.h"

#include "../coro_st_lib/just_exception.h"

#include "../coro_st_lib/co.h"
#include "../coro_st_lib/coro_type_traits.h"

#include "test_loop.h"

#include <stdexcept>

namespace
{
  static_assert(coro_st::is_co_task<coro_st::just_exception_task>);

  TEST(just_exception_chain_root)
  {
    coro_st_test::test_loop tl;

    auto task = coro_st::async_just_exception(std::runtime_error("42"));

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    ASSERT_FALSE(tl.completed);
    ASSERT_FALSE(tl.cancelled);

    awaiter.start_as_chain_root();

    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());
    ASSERT_TRUE(tl.completed);
    ASSERT_FALSE(tl.cancelled);

    ASSERT_TRUE(awaiter.get_result_exception());
    ASSERT_THROW_WHAT(awaiter.await_resume(),std::runtime_error, "42");
  }

  TEST(just_exception_inside_co)
  {
    coro_st_test::test_loop tl;

    auto async_lambda = []() -> coro_st::co<int> {
      co_await coro_st::async_just_exception(std::runtime_error("42"));
      co_return 41;
    };

    auto task = async_lambda();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    ASSERT_FALSE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
    awaiter.start_as_chain_root();
    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());
    tl.run_pending_work();
    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());
    ASSERT_TRUE(tl.completed);
    ASSERT_FALSE(tl.cancelled);
    ASSERT_TRUE(awaiter.get_result_exception());
    ASSERT_THROW_WHAT(awaiter.await_resume(),std::runtime_error, "42");
  }

  // coro_st::co<void> async_just_exception_does_not_compile()
  // {
  //   auto x = coro_st::async_just_exception(std::runtime_error("42"));
  //   // use of deleted function
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_just_exception_does_not_compile2()
  // {
  //   // ignoring return value
  //   coro_st::async_just_exception(std::runtime_error("42"));
  //   co_return;
  // }
} // anonymous namespace