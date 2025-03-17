#pragma once

#include "../coro_lib/st_stop.h"

#include <cassert>
#include <functional>
#include <optional>

namespace coro::st
{
  template <typename T>
  class stop_op_callback_impl
  {
  public:
    using FnPtr = void (T::*)() noexcept;
  private:
    FnPtr fn_;
    T* x_;
  public:
    stop_op_callback_impl() noexcept :
      fn_{ nullptr }, x_{nullptr}
    {
    }

    stop_op_callback_impl(FnPtr fn, T* x) noexcept :
      fn_{ fn }, x_{x}
    {
    }

    stop_op_callback_impl(const stop_op_callback_impl&) = default;
    stop_op_callback_impl& operator=(const stop_op_callback_impl&) = default;

    void operator()() noexcept
    {
      assert(x_ != nullptr);
      std::invoke(fn_, x_);
    }
  };

  template <typename T>
  class stop_op_callback
  {
  public:
    using StopOpImpl = stop_op_callback_impl<T>;
  private:
    std::optional<stop_callback<StopOpImpl>> sc_{};
  public:
    stop_op_callback() noexcept = default;

    stop_op_callback(const stop_op_callback&) = delete;
    stop_op_callback& operator=(const stop_op_callback&) = delete;

    void enable(coro::st::stop_token token, StopOpImpl::FnPtr fn, T* x) noexcept
    {
      sc_.emplace(token, StopOpImpl{ fn, x });
    }

    void disable() noexcept
    {
      sc_ = std::nullopt;
    }
  };
}