#include "../test_lib/test.h"

#include "../coro_st_lib/co.h"

#include "../coro_st_lib/coro_type_traits.h"
#include "../coro_st_lib/run.h"

#include "test_loop.h"

#include <string>

namespace
{
  static_assert(coro_st::is_co_task<coro_st::co<void>>);
  static_assert(coro_st::is_co_task<coro_st::co<int>>);

  coro_st::co<void> async_foo()
  {
    co_return;
  }

  coro_st::co<std::string> async_bar()
  {
    co_return "abc";
  }

  coro_st::co<std::string> async_buzz()
  {
    co_await async_foo();
    auto x = co_await async_bar();
    co_return x;
  }

  TEST(co_chain_root_run)
  {
    auto result = coro_st::run(async_buzz()).value();
    ASSERT_EQ("abc", result);
  }

  TEST(co_chain_root)
  {
    coro_st_test::test_loop tl;

    auto task = async_buzz();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    awaiter.start();

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());
    tl.run_one_ready();
    ASSERT_TRUE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_TRUE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
  }

  TEST(co_chain_root_cancellation)
  {
    coro_st_test::test_loop tl;

    auto task = async_buzz();

    auto awaiter = task.get_work().get_awaiter(tl.ctx);

    tl.stop_source.request_stop();

    awaiter.start();

    ASSERT_FALSE(tl.el.ready_queue_.empty());
    ASSERT_TRUE(tl.el.timers_heap_.empty());

    ASSERT_FALSE(tl.result_ready);
    ASSERT_FALSE(tl.stopped);
    tl.run_one_ready();
    ASSERT_FALSE(tl.result_ready);
    ASSERT_TRUE(tl.stopped);
  }

  // coro_st::co<void> async_does_not_compile()
  // {
  //   auto x = async_foo();
  //   co_await std::move(x);
  // }

  // coro_st::co<void> async_does_not_compile2()
  // {
  //   // fail to compile because the result is not used
  //   async_foo();
  //   co_return;
  // }

  // coro_st::co<void> async_does_not_compile3()
  // {
  //   auto x = async_foo();
  //   // operator co_await not defined
  //   co_await operator co_await(x);
  // }

  // coro_st::co<void> async_does_not_compile4()
  // {
  //   // error get_awaiter
  //   co_await std::suspend_never();
  // }

  struct X
  {
    int i_;
    X(int i) noexcept :
      i_{ i }
    {
    }

    X(const X&) = delete;
    X& operator=(const X&) = delete;

    int foo() const noexcept
    {
      return i_;
    }
  };

  template<typename T>
  int test_argument_is_value(T x)
  {
    return x.foo();
  }

  TEST(co_await_transform_by_value_trick_test)
  {
    int i = test_argument_is_value(X(42));
    ASSERT_TRUE(i);
  }

  // TEST(value_does_not_compile)
  // {
  //   X x(42);
  //   int i = test_argument_is_value(x);
  //   ASSERT_TRUE(i);
  // }

  // TEST(value_does_not_compile2)
  // {
  //   X x(42);
  //   int i = test_argument_is_value(std::move(x));
  //   ASSERT_TRUE(i);
  // }
} // anonymous namespace