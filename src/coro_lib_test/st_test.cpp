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

  coro::task<int> buzz(coro::st::context & ctx, int i)
  {
    record("start " + std::to_string(i));
    co_await ctx.sleep(std::chrono::seconds(0));
    record("end " + std::to_string(i));
    co_return i;
  }

  coro::task<void> bar(coro::st::context & ctx, int& result)
  {
    record("start bar");
    for (int i = 0 ; i < 3 ; ++i)
    {
      result += co_await buzz(ctx, i);
    }
    record("end bar");
  }

  coro::task<void> foo(coro::st::context & ctx, int& result)
  {
    record("start foo");
    ctx.spawn(bar(ctx, result));
    record("end foo");
    co_return;
  }

  TEST(st_test)
  {
    recorder.clear();

    coro::st::context ctx;

    int result = 0;
    ctx.spawn(foo(ctx, result));

    ASSERT_TRUE(recorder.empty());

    ctx.run();

    std::vector<std::string> expected{
      "start foo", "end foo",
      "start bar",
      "start 0", "end 0",
      "start 1", "end 1",
      "start 2", "end 2",
      "end bar",
    };

    std::variant<int, std::string> x;
    ASSERT_EQ(3, result);
    ASSERT_EQ(expected, recorder);
  }
} // anonymous namespace