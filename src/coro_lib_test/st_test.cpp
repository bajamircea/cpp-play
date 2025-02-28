#include "../test_lib/test.h"

#include "../coro_lib/st.h"

#include <string>
#include <vector>

namespace
{
  std::vector<std::string> recorder;

  void record(std::string message)
  {
    recorder.push_back(message);
  }

  coro::co<void> bar(coro::st::context & ctx, int i)
  {
    record("start " + std::to_string(i));
    co_await ctx.sleep(std::chrono::seconds(0));
    record("end " + std::to_string(i));
  }

  coro::co<void> foo(coro::st::context & ctx)
  {
    record("start foo");
    for (int i = 0 ; i < 3 ; ++i)
    {
      co_await bar(ctx, i);
    }
    record("end foo");
  }

  TEST(st_test_timers)
  {
    recorder.clear();

    coro::st::context ctx;

    ctx.run(coro::deferred_co(foo, std::ref(ctx)));

    std::vector<std::string> expected{
      "start foo",
      "start 0", "end 0",
      "start 1", "end 1",
      "start 2", "end 2",
      "end foo",
    };

    ASSERT_EQ(expected, recorder);
  }
} // anonymous namespace