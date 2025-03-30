#include "../test_lib/test.h"

#include "../coro_st_lib/coro_type_traits.h"

#include "../coro_st_lib/co.h"

#include <string>

namespace
{
  struct test_with_void_await_suspend
  {
    void await_suspend(std::coroutine_handle<>) noexcept
    {
    }
  };

  struct test_with_bool_await_suspend
  {
    bool await_suspend(std::coroutine_handle<>) noexcept
    {
      return true;
    }
  };

  TEST(coro_type_traits_has_void_await_suspend)
  {
    static_assert(coro_st::has_void_await_suspend<test_with_void_await_suspend>);
    static_assert(!coro_st::has_void_await_suspend<test_with_bool_await_suspend>);
    static_assert(!coro_st::has_void_await_suspend<coro_st::co<std::string>>);
    static_assert(!coro_st::has_void_await_suspend<int>);
  }

  TEST(coro_type_traits_has_bool_await_suspend)
  {
    static_assert(!coro_st::has_bool_await_suspend<test_with_void_await_suspend>);
    static_assert(coro_st::has_bool_await_suspend<test_with_bool_await_suspend>);
    static_assert(!coro_st::has_bool_await_suspend<coro_st::co<std::string>>);
    static_assert(!coro_st::has_bool_await_suspend<int>);
  }

  TEST(coro_type_traits_has_symmetric_await_suspend)
  {

    static_assert(!coro_st::has_symmetric_await_suspend<test_with_void_await_suspend>);
    static_assert(!coro_st::has_symmetric_await_suspend<test_with_bool_await_suspend>);
    static_assert(coro_st::has_symmetric_await_suspend<
      coro_st::co_awaitable_awaiter_t<coro_st::co<std::string>>>);
    static_assert(!coro_st::has_symmetric_await_suspend<int>);
  }

  TEST(coro_type_traits_co_awaitable)
  {
    static_assert(coro_st::is_co_awaitable<coro_st::co<std::string>>);

    static_assert(std::is_same_v<
      std::string,
      coro_st::co_awaitable_result_t<coro_st::co<std::string>>>);
    static_assert(std::is_same_v<
      void,
      coro_st::co_awaitable_result_t<coro_st::co<void>>>);
  }
} // anonymous namespace