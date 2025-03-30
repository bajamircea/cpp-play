#pragma once

#include <cassert>
#include <functional>

namespace coro_st
{
  class callback
  {
    void* x_{ nullptr };
    void (*fn_)(void* x) noexcept { nullptr };

  public:
    callback() noexcept = default;
    callback(void* x, void (*fn)(void* x) noexcept) noexcept :
      x_{ x },
      fn_{ fn }
    {
      assert(nullptr != fn_);
    }

    callback(const callback&) noexcept = default;
    callback& operator=(const callback&) noexcept = default;

    void invoke() noexcept
    {
      assert(nullptr != fn_);
      fn_(x_);
    }

    bool is_callable() noexcept
    {
      return nullptr != fn_;
    }
  };

  template<typename T, void (T::*member_fn)() noexcept>
  struct make_callback_impl
  {
    static void invoke(void * x_void) noexcept
    {
      assert(x_void != nullptr);
      T* x = reinterpret_cast<T*>(x_void);
      return std::invoke(member_fn, x);
    }
  };

  template<auto MemberFnPtr, typename T>
  callback make_callback(T* x)
  {
    return callback{ x, &make_callback_impl<T, MemberFnPtr>::invoke };
  }
}