#include "../test_lib/test.h"

#include "../coro_st_lib/stop_when.h"

#include "../coro_st_lib/coro_st.h"

namespace
{
  void nothing_fn(void*) noexcept {}

  template<coro_st::is_co_task CoTask>
  void run2(CoTask co_task)
  {
    coro_st::stop_source main_stop_source;

    coro_st::chain_context chain_ctx{
      main_stop_source.get_token(),
      coro_st::callback{ nullptr, nothing_fn },
      coro_st::callback{ nullptr, nothing_fn }
    };
    coro_st::context ctx{ chain_ctx };

    auto co_awaiter = co_task.get_work().get_awaiter(ctx);

    co_awaiter.start_as_chain_root();
  }

  // For some reason that triggers what I believe to be a false positive
  // on g++ that made me use -Wno-dangling-pointer on g++ -O3 build
  TEST(stop_when_exception2)
  {
    auto async_lambda = []() -> coro_st::co<void> {
      co_return;
    };

    run2(coro_st::async_stop_when(
      coro_st::async_suspend_forever(),
      async_lambda()
    ));
  }

} // anonymous namespace