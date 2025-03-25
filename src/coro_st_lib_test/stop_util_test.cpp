#include "../test_lib/test.h"

#include "../coro_st_lib/stop_util.h"

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
} // anonymous namespace