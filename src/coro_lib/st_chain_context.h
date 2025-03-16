#pragma once

#include "st_stop.h"

#include <coroutine>

namespace coro::st
{
  class chain_context
  {
  public:
    using OnResumeFnPtr = void (*)(void* x, std::coroutine_handle<> coroutine) noexcept;

  private:
    coro::st::stop_token token_;
    OnResumeFnPtr on_resume_fn_{ nullptr };
    void* x_{ nullptr };

  public:
    chain_context(stop_token token, OnResumeFnPtr on_resume_fn, void* x) noexcept :
      token_{ token }, on_resume_fn_{ on_resume_fn }, x_{ x }
    {
    }

    chain_context(const chain_context&) = delete;
    chain_context& operator=(const chain_context&) = delete;

    stop_token get_stop_token() noexcept
    {
      return token_;
    }

    void on_resume(std::coroutine_handle<> coroutine)
    {
      assert(on_resume_fn_ != nullptr);
      on_resume_fn_(x_, coroutine);
    }
  };
}