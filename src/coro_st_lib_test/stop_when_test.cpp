#include "../test_lib/test.h"

#include "../coro_st_lib/stop_when.h"

#include "../coro_st_lib/coro_st.h"

#include "test_loop.h"

namespace
{
  // For some reason that triggers what I believe to be a false positive
  // on g++ that made me use -Wno-dangling-pointer on g++ -O3 build
  TEST(stop_when_exception2)
  {
    auto async_lambda = []() -> coro_st::co<void> {
      throw std::runtime_error("Ups!");
      co_return;
    };

    ASSERT_THROW_WHAT(coro_st::run(coro_st::async_stop_when(
      coro_st::async_suspend_forever(),
      async_lambda()
    )), std::runtime_error, "Ups!");
  }
} // anonymous namespace