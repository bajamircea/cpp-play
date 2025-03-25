#include "../test_lib/test.h"

#include "../coro_st_lib/synthetic_coroutine.h"

#include <exception>

namespace
{
  struct offset_check_co
  {
    struct promise_type
    {
      promise_type() noexcept = default;

      promise_type(const promise_type&) = delete;
      promise_type& operator=(const promise_type&) = delete;

      offset_check_co get_return_object() noexcept
      {
        void * promise_address = this;
        void * frame_address =
          std::coroutine_handle<promise_type>::from_promise(*this).address();
        return { reinterpret_cast<char*>(promise_address) -
          reinterpret_cast<char*>(frame_address) };
      }

      std::suspend_never initial_suspend() noexcept
      {
        return {};
      }

      std::suspend_never final_suspend() noexcept
      {
        return {};
      }

      void return_void() noexcept
      {
      }
  
      void unhandled_exception() noexcept
      {
        std::terminate();
      }
    };

    ptrdiff_t promise_offset_from_handle_address{};

    offset_check_co(ptrdiff_t offset) noexcept :
      promise_offset_from_handle_address{ offset }
    {
    }

    offset_check_co(const offset_check_co&) = delete;
    offset_check_co& operator=(const offset_check_co&) = delete;
  };

  offset_check_co foo()
  {
    co_return;
  }

  TEST(synthetic_coroutine_check_expected_promise_offset)
  {
    offset_check_co x = foo();
    ASSERT_EQ(2 * sizeof(void*), x.promise_offset_from_handle_address);
  }

  TEST(synthetic_coroutine_resume_synthetic_resumable)
  {
    bool done{ false };
    coro_st::synthetic_resume_fn_ptr done_when_resumed =
      +[](void* x) noexcept {
        bool* p_done = reinterpret_cast<bool*>(x);
        *p_done = true;
      };
    coro_st::synthetic_resumable_coroutine_frame root_frame{
      done_when_resumed, &done};

    std::coroutine_handle<> handle = root_frame.get_coroutine_handle();
    handle.resume();
    ASSERT_TRUE(done);
  }
} // anonymous namespace