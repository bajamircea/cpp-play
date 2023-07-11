#pragma once

#include "../cpp_util_lib/unique_handle.h"

#include <cassert>
#include <coroutine>
#include <exception>
#include <stdexcept>
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

  template<typename T>
  class task
  {
  public:
    class promise_type {
      friend task;
      std::coroutine_handle<> parent_coro_;
      std::variant<std::monostate, T, std::exception_ptr> data_;

    public:
      promise_type() noexcept : parent_coro_{}, data_{}
      {
      }
      promise_type(const promise_type &) = delete;
      promise_type& operator=(const promise_type &) = delete;

      task get_return_object() noexcept
      {
        return task(std::coroutine_handle<promise_type>::from_promise(*this));
      }

      std::suspend_always initial_suspend() noexcept
      {
        return {};
      }
      
      struct final_await
      {
        [[nodiscard]] constexpr bool await_ready() const noexcept
        {
          return false;
        }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> task_coro) noexcept
        {
          return task_coro.promise().parent_coro_;
        }

        [[noreturn]] constexpr void await_resume() const noexcept
        {
          // unreachable()
          std::terminate();
        }
      };

      friend final_await;

      final_await final_suspend() noexcept
      {
        return {};
      }

      template<typename U>
      void return_value(U && x)
      {
        data_.template emplace<1>(std::forward<U>(x));
      }

      void unhandled_exception() noexcept
      {
        data_.template emplace<2>(std::current_exception());
      }
    };
  private:
    scoped_coroutine_handle<promise_type> scoped_coro_;

    task(std::coroutine_handle<promise_type> task_coro) noexcept : scoped_coro_(task_coro)
    {
    }
  public:
    [[nodiscard]] constexpr bool await_ready() const noexcept
    {
      return false;
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> parent_coro) noexcept
    {
      assert(!promise().parent_coro_);
      promise().parent_coro_ = parent_coro;
      return scoped_coro_.get();
    }

    T await_resume()
    {
      switch(promise().data_.index())
      {
        case 1:
          return std::move(std::get<1>(promise().data_));
        case 2:
          std::rethrow_exception(std::get<2>(promise().data_));
        default:
          throw std::logic_error("Missing return value from task coroutine");
      }
    }
  private:
    promise_type & promise() noexcept
    {
      return scoped_coro_.handle_reference().promise();
    }
  };

  template<>
  class task<void>
  {
  public:
    class promise_type {
      friend task;
      std::coroutine_handle<> parent_coro_;
      std::exception_ptr exception_;

    public:
      promise_type() noexcept : parent_coro_{}, exception_{}
      {
      }
      promise_type(const promise_type &) = delete;
      promise_type& operator=(const promise_type &) = delete;

      task get_return_object() noexcept
      {
        return task(std::coroutine_handle<promise_type>::from_promise(*this));
      }

      std::suspend_always initial_suspend() noexcept
      {
        return {};
      }
      
      struct final_await
      {
        [[nodiscard]] constexpr bool await_ready() const noexcept
        {
          return false;
        }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> task_coro) noexcept
        {
          return task_coro.promise().parent_coro_;
        }

        constexpr void await_resume() const noexcept
        {
          static_assert(true, "lazy final await_resume should not be called");
        }
      };

      friend final_await;

      final_await final_suspend() noexcept
      {
        return {};
      }

      void return_void()
      {
      }

      void unhandled_exception() noexcept
      {
        exception_ = std::current_exception();
      }
    };
  private:
    scoped_coroutine_handle<promise_type> scoped_coro_;

    task(std::coroutine_handle<promise_type> task_coro) noexcept : scoped_coro_(task_coro)
    {
    }
  public:
    [[nodiscard]] constexpr bool await_ready() const noexcept
    {
      return false;
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> parent_coro) noexcept
    {
      assert(!promise().parent_coro_);
      promise().parent_coro_ = parent_coro;
      return scoped_coro_.get();
    }

    void await_resume()
    {
      if (promise().exception_)
      {
        std::rethrow_exception(promise().exception_);
      }
    }
  private:
    promise_type & promise() noexcept
    {
      return scoped_coro_.handle_reference().promise();
    }
  };
}
