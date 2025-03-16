#include "../test_lib/test.h"

#include "../coro_lib/trampoline_co.h"

#include "../coro_lib/co.h"

#include <string>

namespace
{
  coro::co<std::string> async_bar(const int& i)
  {
    co_return std::to_string(i);
  }

  coro::co<std::string> async_foo()
  {
    std::string x{ "start " };
    for (int i = 0 ; i < 3 ; ++i)
    {
      x += co_await async_bar(i);
    }
    x += " stop";
    co_return x;
  }

  TEST(trampoline_co_happy_path)
  {
    auto trampoline = []()
     -> coro::trampoline_co<std::string>
    {
      co_return co_await async_foo();
    };

    bool done{ false };
    coro::OnTrampolineDoneFnPtr on_done = +[](void* x) noexcept {
      bool* p_done = reinterpret_cast<bool*>(x);
      *p_done = true;
    };
    auto t = trampoline();
    t.set_on_done_fn(on_done, &done);

    ASSERT_FALSE(done);
    t.resume();
    ASSERT_TRUE(done);

    std::string result = t.get_result();

    ASSERT_EQ("start 012 stop", result);
  }
} // anonymous namespace