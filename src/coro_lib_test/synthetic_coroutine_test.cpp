#include "../test_lib/test.h"

#include "../coro_lib/synthetic_coroutine.h"

#include "../coro_lib/co.h"

#include <string>

namespace
{
  TEST(synthetic_coroutine_resume)
  {
    bool done{ false };
    coro::synthetic_resume_fn_ptr done_when_resumed =
      +[](void* x) noexcept {
        bool* p_done = reinterpret_cast<bool*>(x);
        *p_done = true;
      };
    coro::synthetic_resumable_coroutine_frame root_frame{
      done_when_resumed, &done};

    std::coroutine_handle<> handle = root_frame.get_coroutine_handle();
    handle.resume();
    ASSERT_TRUE(done);
  }

  coro::co<void> async_foo()
  {
    co_return;
  }

  coro::co<std::string> async_bar(const int& i)
  {
    co_return std::to_string(i);
  }

  coro::co<std::string> async_buzz()
  {
    co_await async_foo();
    int i = 41;
    auto x = co_await async_bar(i + 1);
    co_return x;
  }

  TEST(synthetic_coroutine_co)
  {
    ASSERT_NE(nullptr, &async_buzz);

    bool done{ false };
    coro::synthetic_resume_fn_ptr done_when_resumed =
      +[](void* x) noexcept {
        bool* p_done = reinterpret_cast<bool*>(x);
        *p_done = true;
      };
    coro::synthetic_resumable_coroutine_frame root_frame{
      done_when_resumed, &done};

    std::coroutine_handle<> root_handle = root_frame.get_coroutine_handle();

    auto co_awaiter = async_buzz().hazmat_get_awaiter();

    ASSERT_FALSE(co_awaiter.await_ready());
    auto child_handle = co_awaiter.await_suspend(root_handle);

    child_handle.resume();

    ASSERT_TRUE(done);
    ASSERT_EQ("42", co_awaiter.await_resume());
  }
} // anonymous namespace