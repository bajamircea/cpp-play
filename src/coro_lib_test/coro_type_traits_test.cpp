#include "../test_lib/test.h"

#include "../coro_lib/coro_type_traits.h"

#include "../coro_lib/co.h"
// TODO: test deferred_co
//#include "../coro_lib/deferred_co.h"

#include "dummy_awaiter_suspend_bool.h"
#include "dummy_awaiter_suspend_symmetric.h"
#include "dummy_awaiter_suspend_void.h"
#include "dummy_co_has_member_co_await.h"
#include "dummy_co_has_member_co_await_rvalue.h"
#include "dummy_co_has_member_co_await_private.h"
#include "dummy_co_has_non_member_co_await.h"

#include <functional>
#include <string>

namespace
{
  coro::co<std::string> async_foo_co() {
    co_return "42";
  };

  coro_test::dummy_co_has_member_co_await<std::string> async_foo() {
    co_return "42";
  };

  coro_test::dummy_co_has_member_co_await_rvalue<std::string> async_foo_rvalue() {
    co_return "42";
  };

  coro_test::dummy_co_has_member_co_await_private<std::string> async_foo_private() {
    co_return "42";
  };

  template<typename Awaitable>
  void can_use_awaitable_with_member_co_await(Awaitable&&)
  {
    static_assert(coro::has_member_operator_co_await<Awaitable>);
  }

  template<typename Awaitable>
  void can_not_use_awaitable_with_member_co_await(Awaitable&&)
  {
    static_assert(!coro::has_member_operator_co_await<Awaitable>);
  }

  TEST(coro_type_traits_has_member_co_await)
  {
    static_assert(coro::has_member_operator_co_await<coro_test::dummy_co_has_member_co_await<int>>);
    static_assert(coro::has_member_operator_co_await<coro_test::dummy_co_has_member_co_await<void>>);

    static_assert(coro::has_member_operator_co_await<coro_test::dummy_co_has_member_co_await_rvalue<int>>);
    static_assert(coro::has_member_operator_co_await<coro_test::dummy_co_has_member_co_await_rvalue<void>>);

    static_assert(!coro::has_member_operator_co_await<coro_test::dummy_co_has_member_co_await_private<int>>);

    static_assert(!coro::has_member_operator_co_await<coro_test::dummy_co_has_non_member_co_await<int>>);

    static_assert(!coro::has_member_operator_co_await<coro::co<int>>);

    auto foo = async_foo();
    static_assert(coro::has_member_operator_co_await<decltype(foo)>);
    can_use_awaitable_with_member_co_await(foo);

    auto invoked_lambda_has_member = std::invoke([]()
      -> coro_test::dummy_co_has_member_co_await<void>
      { co_return;
    });
    static_assert(coro::has_member_operator_co_await<decltype(invoked_lambda_has_member)>);
    can_use_awaitable_with_member_co_await(invoked_lambda_has_member);

    auto invoked_lambda_co = std::invoke([]()
      -> coro::co<void>
      { co_return;
    });
    static_assert(!coro::has_member_operator_co_await<decltype(invoked_lambda_co)>);
    can_not_use_awaitable_with_member_co_await(invoked_lambda_co);

    auto foo_rvalue = async_foo_rvalue();
    static_assert(coro::has_member_operator_co_await<decltype(foo_rvalue)>);
    can_use_awaitable_with_member_co_await(foo_rvalue);

    can_use_awaitable_with_member_co_await(async_foo());

    can_use_awaitable_with_member_co_await(async_foo_rvalue());

    can_not_use_awaitable_with_member_co_await(async_foo_private());

    static_assert(!coro::has_member_operator_co_await<std::suspend_always>);

    static_assert(!coro::has_member_operator_co_await<int>);
    can_not_use_awaitable_with_member_co_await(42);

    static_assert(!coro::has_member_operator_co_await<std::string>);
    can_not_use_awaitable_with_member_co_await(std::string());

    auto la = []() -> void{};
    static_assert(!coro::has_member_operator_co_await<decltype(la)>);
    static_assert(!coro::has_member_operator_co_await<decltype(la())>);

    auto la_has_member_co_await = []() ->
      coro_test::dummy_co_has_member_co_await<void>
      {
        co_return;
      };
    static_assert(!coro::has_member_operator_co_await<decltype(la_has_member_co_await)>);
    static_assert(coro::has_member_operator_co_await<decltype(la_has_member_co_await())>);

    auto la_has_member_co_await_rvalue = []() ->
      coro_test::dummy_co_has_member_co_await_rvalue<void>
      {
        co_return;
      };
    static_assert(!coro::has_member_operator_co_await<decltype(la_has_member_co_await_rvalue)>);
    static_assert(coro::has_member_operator_co_await<decltype(la_has_member_co_await_rvalue())>);

    auto la_has_member_co_await_private = []() ->
    coro_test::dummy_co_has_member_co_await_private<void>
    {
      co_return;
    };
    static_assert(!coro::has_member_operator_co_await<decltype(la_has_member_co_await_private)>);
    static_assert(!coro::has_member_operator_co_await<decltype(la_has_member_co_await_private())>);
  }

  TEST(coro_type_traits_has_non_member_co_await)
  {
    static_assert(!coro::has_non_member_operator_co_await<coro_test::dummy_co_has_member_co_await<int>>);

    static_assert(!coro::has_non_member_operator_co_await<coro_test::dummy_co_has_member_co_await_rvalue<int>>);

    static_assert(!coro::has_non_member_operator_co_await<coro_test::dummy_co_has_member_co_await_private<int>>);

    static_assert(coro::has_non_member_operator_co_await<coro_test::dummy_co_has_non_member_co_await<int>>);

    // it has, but it's private
    static_assert(!coro::has_non_member_operator_co_await<coro::co<int>>);

    static_assert(!coro::has_non_member_operator_co_await<std::string>);
  }

  TEST(coro_type_traits_has_get_awaiter)
  {
    static_assert(!coro::has_get_awaiter<coro_test::dummy_co_has_member_co_await<int>>);

    static_assert(!coro::has_get_awaiter<coro_test::dummy_co_has_member_co_await_rvalue<int>>);

    static_assert(!coro::has_get_awaiter<coro_test::dummy_co_has_member_co_await_private<int>>);

    static_assert(!coro::has_get_awaiter<coro_test::dummy_co_has_non_member_co_await<int>>);

    static_assert(coro::has_get_awaiter<coro::co<int>>);
    static_assert(coro::has_get_awaiter<coro::co<void>>);

    static_assert(!coro::has_get_awaiter<std::string>);
  }

  TEST(coro_type_traits_get_awaiter)
  {
    auto awaiter_member_co_await = coro::get_awaiter(async_foo());

    auto awaitable_member_co_await_variable = async_foo();
    auto awaiter_member_co_await2 = coro::get_awaiter(awaitable_member_co_await_variable);

    auto awaiter_member_co_await_rvalue = coro::get_awaiter(async_foo_rvalue());

    auto awaitable_member_co_await_rvalue_variable = async_foo_rvalue();
    auto awaiter_member_co_await_rvalue2 = coro::get_awaiter(awaitable_member_co_await_rvalue_variable);

    auto awaiter_co = coro::get_awaiter(async_foo_co());

    auto awaitable_co_variable = async_foo_co();
    auto awaiter_co2 = coro::get_awaiter(awaitable_co_variable);
  }

  TEST(coro_type_traits_has_void_await_suspend)
  {
    static_assert(!coro::has_void_await_suspend<coro_test::dummy_awaiter_suspend_bool>);
    static_assert(!coro::has_void_await_suspend<coro_test::dummy_awaiter_suspend_symmetric>);
    static_assert(coro::has_void_await_suspend<coro_test::dummy_awaiter_suspend_void>);
    auto awaitable_co_variable = async_foo_co();
    auto awaiter_co2 = coro::get_awaiter(awaitable_co_variable);
    static_assert(!coro::has_void_await_suspend<decltype(awaiter_co2)>);
  }

  TEST(coro_type_traits_has_bool_await_suspend)
  {
    static_assert(coro::has_bool_await_suspend<coro_test::dummy_awaiter_suspend_bool>);
    static_assert(!coro::has_bool_await_suspend<coro_test::dummy_awaiter_suspend_symmetric>);
    static_assert(!coro::has_bool_await_suspend<coro_test::dummy_awaiter_suspend_void>);
    auto awaitable_co_variable = async_foo_co();
    auto awaiter_co2 = coro::get_awaiter(awaitable_co_variable);
    static_assert(!coro::has_bool_await_suspend<decltype(awaiter_co2)>);
  }

  TEST(coro_type_traits_has_symmetric_await_suspend)
  {
    static_assert(!coro::has_symmetric_await_suspend<coro_test::dummy_awaiter_suspend_bool>);
    static_assert(coro::has_symmetric_await_suspend<coro_test::dummy_awaiter_suspend_symmetric>);
    static_assert(!coro::has_symmetric_await_suspend<coro_test::dummy_awaiter_suspend_void>);
    auto awaitable_co_variable = async_foo_co();
    auto awaiter_co2 = coro::get_awaiter(awaitable_co_variable);
    static_assert(coro::has_symmetric_await_suspend<decltype(awaiter_co2)>);
  }

  TEST(coro_type_traits_is_awaiter)
  {
    static_assert(coro::is_awaiter<coro_test::dummy_awaiter_suspend_bool>);
    static_assert(coro::is_awaiter<coro_test::dummy_awaiter_suspend_symmetric>);
    static_assert(coro::is_awaiter<coro_test::dummy_awaiter_suspend_void>);
    auto awaitable_co_variable = async_foo_co();
    auto awaiter_co2 = coro::get_awaiter(awaitable_co_variable);
    static_assert(coro::is_awaiter<decltype(awaiter_co2)>);
  }

  TEST(coro_type_traits_awaitable_traits)
  {
    static_assert(std::is_same_v<
      std::string,
      coro::awaitable_traits<decltype(async_foo_co())>::await_result_t>);

    static_assert(std::is_same_v<
      void,
      coro::awaitable_traits<coro_test::dummy_awaiter_suspend_bool>::await_result_t>);
  }
} // anonymous namespace