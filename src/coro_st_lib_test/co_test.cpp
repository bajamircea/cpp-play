// /std:c++20 /fsanitize=address

#include <cassert>
#include <coroutine>
#include <exception>
#include <functional>
#include <memory>

namespace coro_st
{
  class callback
  {
    using pure_callback_fn = void (*)(void* x) noexcept;

    void* x_{ nullptr };
    pure_callback_fn fn_ { nullptr };

  public:
    callback() noexcept = default;
    callback(void* x, pure_callback_fn fn) noexcept :
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

  struct [[nodiscard]] co
  {
    struct promise_type
    {
      callback* cb_{ nullptr };
    public:
      promise_type() noexcept = default;

      promise_type(const promise_type&) = delete;
      promise_type& operator=(const promise_type&) = delete;

      co get_return_object() noexcept
      {
        return {std::coroutine_handle<promise_type>::from_promise(*this)};
      }

      void return_void() noexcept
      {
      }

      void unhandled_exception() noexcept
      {
        std::terminate();
      }

      std::suspend_always initial_suspend() noexcept
      {
        return {};
      }

      struct final_awaiter
      {
        callback* cb_;

        [[nodiscard]] constexpr bool await_ready() const noexcept
        {
          return false;
        }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type>) noexcept
        {
          assert(nullptr != cb_);
          cb_->invoke();
          return std::noop_coroutine();
        }

        [[noreturn]] void await_resume() const noexcept
        {
          std::terminate();
        }
      };

      final_awaiter final_suspend() noexcept
      {
        assert(cb_ != nullptr);
        return {cb_};
      }
    };

    std::coroutine_handle<promise_type> child_coro_{};

    co(std::coroutine_handle<promise_type> child_coro) noexcept :
      child_coro_{ child_coro }
    {
    }
    ~co()
    {
      if (child_coro_)
      {
        child_coro_.destroy();
      }
    }

    co(const co&) = delete;
    co& operator=(const co&) = delete;
    co(co&& other) noexcept :
      child_coro_{ other.child_coro_ }
    {
      other.child_coro_ = {};
    }

    void start(callback& cb) noexcept
    {
      child_coro_.promise().cb_ = &cb;
      child_coro_.resume();
    }
  };
}

coro_st::co async_do_nothing()
{
  co_return;
}

int main()
{
  std::unique_ptr<coro_st::co> coro_ptr = std::make_unique<coro_st::co>(async_do_nothing());
  coro_st::callback on_completed_destroy =
    coro_st::make_function_callback<+[](std::unique_ptr<coro_st::co>& x) noexcept {
      x.reset();
    }>(coro_ptr);
  coro_ptr->start(on_completed_destroy);
  return 0;
}
