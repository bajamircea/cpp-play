#include "../test_lib/test.h"

#include "../coro_st_lib/stop_util.h"

#include "../coro_st_lib/callback.h"

#include <optional>

#include <stop_token>
#include <iostream>

namespace
{
  TEST(stop_util_trivial)
  {
    coro_st::stop_source source;

    coro_st::stop_token token = source.get_token();

    ASSERT_FALSE(source.stop_requested());
    ASSERT_FALSE(token.stop_requested());

    bool called{ false };
    coro_st::stop_callback callback{ token, [&called]() noexcept {
      called = true;
    }};

    ASSERT_FALSE(called);

    {
      bool not_called{ false };
      coro_st::stop_callback not_called_callback{ token, [&not_called]() noexcept {
        not_called = true;
      }};
      ASSERT_FALSE(not_called);
    }

    ASSERT_TRUE(source.request_stop());

    ASSERT_TRUE(called);

    bool called2{ false };
    coro_st::stop_callback callback2{ token, [&called2]() noexcept {
      called2 = true;
    }};

    ASSERT_TRUE(called2);

    ASSERT_TRUE(source.stop_requested());
    ASSERT_TRUE(token.stop_requested());

    ASSERT_FALSE(source.request_stop());
    ASSERT_TRUE(source.stop_requested());
    ASSERT_TRUE(token.stop_requested());
  }

  TEST(stop_util_stop_callback_of_callback)
  {
    coro_st::stop_source source;

    coro_st::stop_token token = source.get_token();

    bool called{ false };
    coro_st::callback cb{ coro_st::make_function_callback<+[](bool& x) noexcept {
      x = true;
    }>(called) };

    std::optional<coro_st::stop_callback<coro_st::callback>> stop_cb;
    stop_cb.emplace(token, std::move(cb));

    ASSERT_FALSE(called);
    source.request_stop();
    ASSERT_TRUE(called);
  }

  // TEST(stop_util_size)
  // {
  //   coro_st::stop_source source;

  //   coro_st::stop_token token = source.get_token();

  //   bool called{ false };
  //   coro_st::callback cb{ coro_st::make_function_callback<+[](bool& x) noexcept {
  //     x = true;
  //   }>(called)};

  //   coro_st::stop_callback<coro_st::callback> coro_st_stop_with_callback(token, std::move(cb));
  //   std::cout << sizeof(coro_st_stop_with_callback) << '\n';

  //   coro_st::stop_source source1;

  //   coro_st::stop_token token1 = source1.get_token();

  //   bool called1{ false };
  //   auto cb1 = [&called1]() noexcept {
  //     called1 = true;
  //   };

  //   coro_st::stop_callback<decltype(cb1)> coro_st_stop_with_callback1(token1, std::move(cb1));
  //   std::cout << sizeof(coro_st_stop_with_callback1) << '\n';

  //   std::stop_source source2;

  //   std::stop_token token2 = source2.get_token();

  //   bool called2{ false };
  //   auto cb2 = [&called2]() noexcept {
  //     called2 = true;
  //   };

  //   std::stop_callback<decltype(cb2)> coro_st_stop_with_callback2(token2, std::move(cb2));
  //   std::cout << sizeof(coro_st_stop_with_callback2) << '\n';
  // }
} // anonymous namespace