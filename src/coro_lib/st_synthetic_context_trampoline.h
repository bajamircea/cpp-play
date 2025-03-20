#pragma once

#include "coro_type_traits.h"
#include "synthetic_coroutine.h"
#include "st_type_traits.h"

namespace coro::st
{
  // TODO: high come back after traits are sorted out
  template<typename DeferredCoFn>
  class st_synthetic_context_trampoline
  {
    using Awaitable = std::invoke_result_t()

    DeferredCoFn fn_;

  public:
    synthetic_resumable_coroutine_frame(synthetic_resume_fn_ptr resume_fn, void* x) noexcept :
      coroutine_frame_abi{ .resume=resume_impl, .destroy=destroy_impl },
      no_parent_marker_{ nullptr },
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