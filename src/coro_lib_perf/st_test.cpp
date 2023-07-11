#include "../test_lib/test.h"

#include "../coro_lib/st.h"

#include <string>
#include <vector>

namespace
{
  std::vector<std::string> recorder;

  // void record(std::string message)
  // {
  //   recorder.push_back(message);
  // }

  coro::task<void> bar(coro::st::context & ctx, int& count)
  {
    co_await ctx.sleep(std::chrono::seconds(1));
    ++count;
  }

  const int g_max_count = 10 * 1000 * 1000;

  coro::task<void> foo(coro::st::context & ctx, int& count)
  {
    for (int i = 0 ; i < g_max_count ; ++i)
    {
      ctx.spawn(bar(ctx, count));
    }
    co_return;
  }

  TEST(st_test_create_timers)
  {
    recorder.clear();

    coro::st::context ctx;

    int count = 0;

    ctx.spawn(foo(ctx, count));

    ASSERT_TRUE(recorder.empty());

    ctx.run();

    ASSERT_EQ(count, g_max_count);
  }
} // anonymous namespace