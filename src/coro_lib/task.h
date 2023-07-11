#pragma once

#include "../cpp_util_lib/unique_handle.h"

#include <cassert>
#include <coroutine>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <variant>

namespace coro
{
  template<typename Promise>
  struct scoped_coroutine_handle_traits
  {
    using handle_type = std::coroutine_handle<Promise>;
    static constexpr auto invalid_value() noexcept { return nullptr; }
    static constexpr auto is_valid(handle_type h) noexcept { return h.operator bool(); }
    static void close_handle(handle_type h) noexcept { h.destroy(); }
  };
  template<typename Promise>
  using scoped_coroutine_handle = cpp_util::unique_handle<scoped_coroutine_handle_traits<Promise>>;

  template<typename Promise>
  struct task_promise_final_await
  {
    [[nodiscard]] constexpr bool await_ready() const noexcept
    {
      return false;
    }

    auto await_suspend(std::coroutine_handle<Promise> promise_coro) noexcept
    {
      return promise_coro.promise().final_await_suspend();
    }

    [[noreturn]] constexpr void await_resume() const noexcept
    {
      std::terminate();
    }
  };

  template<typename T>
  class task;

  template<typename T>
  struct task_promise
  {
    std::coroutine_handle<> continuation_coro_;
    std::variant<std::monostate, T, std::exception_ptr> result_;

    task_promise() noexcept : continuation_coro_{}, result_{}
    {
    }

    task<T> get_return_object() noexcept;

    std::suspend_always initial_suspend() noexcept
    {
      return {};
    }

    task_promise_final_await<task_promise> final_suspend() noexcept
    {
      return {};
    }

    std::coroutine_handle<> final_await_suspend() noexcept
    {
      return continuation_coro_;
    }

    template<typename U>
      requires std::convertible_to<U, T>
    void return_value(U && x) noexcept(std::is_nothrow_constructible_v<T, U>)
    {
      result_.template emplace<1>(std::forward<U>(x));
    }

    void unhandled_exception() noexcept
    {
      result_.template emplace<2>(std::current_exception());
    }

    T get_result()
    {
      switch(result_.index())
      {
        case 1:
          return std::move(std::get<1>(result_));
        case 2:
          std::rethrow_exception(std::get<2>(result_));
        default:
          std::terminate();
      }
    }
  };

  template<>
  struct task_promise<void>
  {
    std::coroutine_handle<> continuation_coro_;
    std::exception_ptr exception_;

    task_promise() noexcept : continuation_coro_{}, exception_{}
    {
    }

    task<void> get_return_object() noexcept;

    std::suspend_always initial_suspend() noexcept
    {
      return {};
    }

    task_promise_final_await<task_promise> final_suspend() noexcept
    {
      return {};
    }

    std::coroutine_handle<> final_await_suspend() noexcept
    {
      return continuation_coro_;
    }

    void return_void() noexcept
    {
    }

    void unhandled_exception() noexcept
    {
      exception_ = std::current_exception();
    }

    void get_result()
    {
      if (exception_)
      {
        std::rethrow_exception(exception_);
      }
    }
  };

  template<typename Task>
  struct task_awaiter
  {
    std::coroutine_handle<typename Task::promise_type> task_coro;

    [[nodiscard]] constexpr bool await_ready() const noexcept
    {
      return false;
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation_coro) noexcept
    {
      assert(!task_coro.promise().continuation_coro_);
      task_coro.promise().continuation_coro_ = continuation_coro;
      return task_coro;
    }

    auto await_resume()
    {
      return task_coro.promise().get_result();
    }
  };

  template<typename T>
  class [[nodiscard]] task
  {
  public:
    using result_type = T;
    using promise_type = task_promise<T>;

  private:
    scoped_coroutine_handle<promise_type> task_coro_;

    friend struct task_promise<T>;

    task(std::coroutine_handle<promise_type> task_coro) noexcept : task_coro_(task_coro)
    {
    }
  public:
    auto operator co_await() &&
    {
      return task_awaiter<task>{task_coro_.get()};
    }
  };

  template<typename T>
  task<T> task_promise<T>::get_return_object() noexcept
  {
      return task<T>{std::coroutine_handle<task_promise<T>>::from_promise(*this)};
  }

  task<void> task_promise<void>::get_return_object() noexcept
  {
      return task<void>{std::coroutine_handle<task_promise<void>>::from_promise(*this)};
  }
}
