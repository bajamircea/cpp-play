#include "../test_lib/test.h"

#include "../coro_st_lib/completion.h"

#include <exception>

namespace
{
  TEST(completion_function_result_ready)
  {
    coro_st::completion c;

    struct completion_flags
    {
      bool result_ready{ false };
      bool stopped{ false };
    };
    completion_flags cf;

    c = coro_st::make_function_completion<
      +[](completion_flags & x) noexcept { x.result_ready = true;},
      +[](completion_flags & x) noexcept { x.stopped = true;}
    >(cf);

    ASSERT_FALSE(cf.result_ready);
    ASSERT_FALSE(cf.stopped);

    c.get_result_ready_callback().invoke();

    ASSERT_TRUE(cf.result_ready);
    ASSERT_FALSE(cf.stopped);
  }

  TEST(completion_function_stopped)
  {
    coro_st::completion c;

    struct completion_flags
    {
      bool result_ready{ false };
      bool stopped{ false };
    };
    completion_flags cf;

    c = coro_st::make_function_completion<
      +[](completion_flags & x) noexcept { x.result_ready = true;},
      +[](completion_flags & x) noexcept { x.stopped = true;}
      >(cf);

    ASSERT_FALSE(cf.result_ready);
    ASSERT_FALSE(cf.stopped);

    c.get_stopped_callback().invoke();

    ASSERT_FALSE(cf.result_ready);
    ASSERT_TRUE(cf.stopped);
  }

  TEST(callback_member_result_ready)
  {
    coro_st::completion c;

    bool result_ready{ false };
    bool stopped{ false };

    struct X
    {
      bool& result_ready;
      bool& stopped;

      void set_result_ready() noexcept
      {
        result_ready = true;
      }
      void set_stopped() noexcept
      {
        stopped = true;
      }
    };

    X x{ result_ready, stopped };

    c = coro_st::make_member_completion<
      &X::set_result_ready,
      &X::set_stopped
      >(&x);

    ASSERT_FALSE(result_ready);
    ASSERT_FALSE(stopped);

    c.get_result_ready_callback().invoke();

    ASSERT_TRUE(result_ready);
    ASSERT_FALSE(stopped);
  }

  TEST(callback_member_stopped)
  {
    coro_st::completion c;

    bool result_ready{ false };
    bool stopped{ false };

    struct X
    {
      bool& result_ready;
      bool& stopped;

      void set_result_ready() noexcept
      {
        result_ready = true;
      }
      void set_stopped() noexcept
      {
        stopped = true;
      }
    };

    X x{ result_ready, stopped };

    c = coro_st::make_member_completion<
      &X::set_result_ready,
      &X::set_stopped
      >(&x);

    ASSERT_FALSE(result_ready);
    ASSERT_FALSE(stopped);

    c.get_stopped_callback().invoke();

    ASSERT_FALSE(result_ready);
    ASSERT_TRUE(stopped);
  }
} // anonymous namespace