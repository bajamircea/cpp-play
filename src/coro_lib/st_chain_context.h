#pragma once

#include "st_stop.h"

#include <coroutine>

namespace coro::st
{
  template <typename Fn>
  class custom_chain_context;

  class chain_context
  {
    template <typename Fn>
    friend class custom_chain_context;
  public:
    using OnResumeFnPtr = void (*)(void* self, std::coroutine_handle<> coroutine) noexcept;
  private:
    coro::st::stop_token token_;
    OnResumeFnPtr on_resume_fn_{ nullptr };

    explicit chain_context(stop_token token) noexcept :
      token_{ token }
    {
    }

  public:
    chain_context(const chain_context&) = delete;
    chain_context& operator=(const chain_context&) = delete;

    stop_token get_stop_token() noexcept
    {
      return token_;
    }

    void on_resume(std::coroutine_handle<> coroutine)
    {
      assert(on_resume_fn_ != nullptr);
      on_resume_fn_(this, coroutine);
    }

  private:
    void set_fn(OnResumeFnPtr on_resume_fn) noexcept
    {
      on_resume_fn_ = on_resume_fn;
    }
  };

  template <typename Fn>
  class custom_chain_context : public chain_context
  {
    Fn fn_;
  public:
    custom_chain_context(stop_token token, Fn&& fn) :
      chain_context{ token }, fn_{ std::move(fn) }
    {
      set_fn(invoke);
    }

    custom_chain_context(const custom_chain_context&) = delete;
    custom_chain_context& operator=(const custom_chain_context&) = delete;

  private:
    static void invoke(void* x, std::coroutine_handle<> coroutine) noexcept
    {
      assert(x != nullptr);
      custom_chain_context* self = reinterpret_cast<custom_chain_context*>(x);
      self->fn_(coroutine);
    }
  };
}