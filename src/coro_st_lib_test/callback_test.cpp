#include "../test_lib/test.h"

#include "../coro_st_lib/callback.h"

#include "../coro_st_lib/unique_coroutine_handle.h"

#include <exception>

namespace
{
  TEST(callback_trivial_function)
  {
    coro_st::callback cb;

    bool called{ false };

    cb = coro_st::make_function_callback<+[](bool & x) noexcept {
      x = true;
    }>(called);

    ASSERT_FALSE(called);

    cb.invoke();

    ASSERT_TRUE(called);
  }

  TEST(callback_trivial_member)
  {
    coro_st::callback cb;

    bool called{ false };

    struct X
    {
      bool& called;

      void set_called() noexcept
      {
        called = true;
      }
    };

    X x{ called };

    cb = coro_st::make_member_callback<&X::set_called>(&x);

    ASSERT_FALSE(called);

    cb.invoke();

    ASSERT_TRUE(called);
  }

  struct dummy_co
  {
    struct promise_type
    {
      promise_type() noexcept = default;

      promise_type(const promise_type&) = delete;
      promise_type& operator=(const promise_type&) = delete;

      dummy_co get_return_object() noexcept
      {
        return {std::coroutine_handle<promise_type>::from_promise(*this)};
      }

      std::suspend_always initial_suspend() noexcept
      {
        return {};
      }

      std::suspend_always final_suspend() noexcept
      {
        return {};
      }

      void return_void() noexcept
      {
      }

      void unhandled_exception() noexcept
      {
        std::terminate();
      }
    };

    coro_st::unique_coroutine_handle<promise_type> unique_child_coro;

    dummy_co(std::coroutine_handle<promise_type> child_coro) noexcept :
      unique_child_coro{ child_coro }
    {
    }

    dummy_co(const dummy_co&) = delete;
    dummy_co& operator=(const dummy_co&) = delete;
  };

  dummy_co async_foo(bool& called)
  {
    called = true;
    co_return;
  }

  TEST(callback_resume)
  {
    bool called{ false };
    dummy_co co = async_foo(called);

    coro_st::callback cb =
      coro_st::make_resume_coroutine_callback(co.unique_child_coro.get());

    ASSERT_FALSE(called);

    cb.invoke();

    ASSERT_TRUE(called);
  }
} // anonymous namespace