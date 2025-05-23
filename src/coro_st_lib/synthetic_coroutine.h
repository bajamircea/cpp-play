#pragma once

#include <cassert>
#include <coroutine>
#include <utility>

namespace coro_st
{
  struct coroutine_frame_abi
  {
    void (*resume)(void*);
    void (*destroy)(void*);
  };

  using synthetic_resume_fn_ptr = void (*)(void* x) noexcept;

  class synthetic_resumable_coroutine_frame : public coroutine_frame_abi
  {
    synthetic_resume_fn_ptr resume_fn_{ nullptr };
    void* x_{ nullptr };
  public:
    synthetic_resumable_coroutine_frame(synthetic_resume_fn_ptr resume_fn, void* x) noexcept :
      coroutine_frame_abi{ .resume=resume_impl, .destroy=destroy_impl },
      resume_fn_{ resume_fn },
      x_{ x }
    {
      assert(nullptr != resume_fn);
    }

    synthetic_resumable_coroutine_frame(const synthetic_resumable_coroutine_frame&) = delete;
    synthetic_resumable_coroutine_frame& operator=(const synthetic_resumable_coroutine_frame&) = delete;

    std::coroutine_handle<> get_coroutine_handle() noexcept
    {
      return std::coroutine_handle<>::from_address(this);
    }

  private:
    static void resume_impl(void* frame_ptr) noexcept
    {
      assert(frame_ptr != nullptr);
      synthetic_resumable_coroutine_frame* self =
        reinterpret_cast<synthetic_resumable_coroutine_frame*>(frame_ptr);
      self->resume_fn_(self->x_);
    }

    static void destroy_impl(void*) noexcept
    {
      std::unreachable();
      //std::terminate();
    }
  };
}