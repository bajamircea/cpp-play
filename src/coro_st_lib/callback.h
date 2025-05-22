#pragma once

#include <cassert>
#include <coroutine>
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

    void operator()() noexcept
    {
      invoke();
    }

    bool is_callable() noexcept
    {
      return nullptr != fn_;
    }
  };

  template<typename T, void (*fn)(T&) noexcept>
  struct make_function_callback_impl
  {
    static void invoke(void* x_void) noexcept
    {
      assert(x_void != nullptr);
      T* x = reinterpret_cast<T*>(x_void);
      return std::invoke(fn, *x);
    }
  };

  template<auto FnPtr, typename T>
  callback make_function_callback(T& x)
  {
    return callback{ &x, &make_function_callback_impl<T, FnPtr>::invoke };
  }

  template<typename T, void (T::*member_fn)() noexcept>
  struct make_member_callback_impl
  {
    static void invoke(void* x_void) noexcept
    {
      assert(x_void != nullptr);
      T* x = reinterpret_cast<T*>(x_void);
      return std::invoke(member_fn, x);
    }
  };

  template<auto MemberFnPtr, typename T>
  callback make_member_callback(T* x)
  {
    return callback{ x, &make_member_callback_impl<T, MemberFnPtr>::invoke };
  }

  inline callback make_resume_coroutine_callback(std::coroutine_handle<> handle)
  {
    return callback{ handle.address(), +[](void* x) noexcept {
      std::coroutine_handle<> original_handle = std::coroutine_handle<>::from_address(x);
      original_handle.resume();
    }};
  }
}