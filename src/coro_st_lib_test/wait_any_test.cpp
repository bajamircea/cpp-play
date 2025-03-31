#include "../test_lib/test.h"

#include "../coro_st_lib/wait_any.h"

#include "../coro_st_lib/co.h"
#include "../coro_st_lib/run.h"

// namespace
// {
//   TEST(yield_chain_root)
//   {
//     coro_st::run(coro_st::async_yield());
//   }

//   TEST(yield_lambda_return_int)
//   {
//     auto async_lambda = []() -> coro_st::co<int> {
//       co_await coro_st::async_yield();
//       co_return 42;
//     };
//     int result = coro_st::run(async_lambda());

//     ASSERT_EQ(42, result);
//   }

//   // coro_st::co<void> async_yield_does_not_compile()
//   // {
//   //   auto x = coro_st::async_yield();
//   //   co_await std::move(x);
//   // }

//   // coro_st::co<void> async_yield_does_not_compile2()
//   // {
//   //   coro_st::async_yield();
//   //   co_return;
//   // }
// } // anonymous namespace