#pragma once

#include "callback.h"

#include <cassert>

namespace coro_st
{
  class completion
  {
    using pure_callback_fn = void (*)(void* x) noexcept;

    void* x_{ nullptr };
    pure_callback_fn result_ready_fn_ { nullptr };
    pure_callback_fn stopped_fn_ { nullptr };

  public:
    completion() noexcept = default;
    completion(
      void* x,
      pure_callback_fn result_ready_fn,
      pure_callback_fn stopped_fn
    ) noexcept :
      x_{ x },
      result_ready_fn_{ result_ready_fn },
      stopped_fn_{ stopped_fn }
    {
      assert((nullptr != result_ready_fn_) && (nullptr != stopped_fn_));
    }

    completion(const completion&) noexcept = default;
    completion& operator=(const completion&) noexcept = default;

    callback get_result_ready_callback() noexcept
    {
      assert(nullptr != result_ready_fn_);
      return {x_, result_ready_fn_};
    }

    callback get_stopped_callback() noexcept
    {
      assert(nullptr != stopped_fn_);
      return {x_, stopped_fn_};
    }

    bool has_callbacks() noexcept
    {
      return nullptr != result_ready_fn_;
    }
  };

  template<auto ResultReadyFnPtr, auto StoppedFnPtr, typename T>
  completion make_function_completion(T& x)
  {
    return completion{ &x,
      &make_function_callback_impl<T, ResultReadyFnPtr>::invoke,
      &make_function_callback_impl<T, StoppedFnPtr>::invoke };
  }

  template<auto ResultReadyMemberFnPtr, auto StoppedMemberFnPtr, typename T>
  completion make_member_completion(T* x)
  {
    return completion{ x,
      &make_member_callback_impl<T, ResultReadyMemberFnPtr>::invoke,
      &make_member_callback_impl<T, StoppedMemberFnPtr>::invoke };
  }
}