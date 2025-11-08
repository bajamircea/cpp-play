#pragma once

#include "context.h"

#include <cassert>
#include <coroutine>
#include <utility>

namespace coro_st
{
  template<typename Promise>
  class [[nodiscard]] unique_coroutine_handle
  {
  public:
    using handle_type = std::coroutine_handle<Promise>;

  private:
    handle_type h_;

  public:
    unique_coroutine_handle() noexcept :
      h_{ nullptr }
    {
    }

    explicit unique_coroutine_handle(const handle_type& h) noexcept :
      h_{ h }
    {
    }

    explicit unique_coroutine_handle(handle_type&& h) noexcept :
      h_{ std::move(h) }
    {
    }

    template<typename Arg1, typename Arg2, typename ... Args>
      requires(std::is_nothrow_constructible_v<handle_type, Arg1, Arg2, Args...>)
    unique_coroutine_handle(Arg1 && arg1, Arg2 && arg2, Args && ... args) noexcept
      :
      h_{ std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Args>(args) ... }
    {
    }

    ~unique_coroutine_handle()
    {
      close_if_valid();
    }

    unique_coroutine_handle(const unique_coroutine_handle &) = delete;
    unique_coroutine_handle & operator=(const unique_coroutine_handle &) = delete;

    unique_coroutine_handle(unique_coroutine_handle && other) noexcept :
      h_{ std::move(other.h_) }
    {
      other.h_ = nullptr;
    }

    unique_coroutine_handle & operator=(unique_coroutine_handle && other) noexcept
    {
      handle_type tmp = std::move(other.h_);
      other.h_ = nullptr;
      close_if_valid();
      h_ = std::move(tmp);
      return *this;
    }

    handle_type get() const noexcept
    {
      return h_;
    }

    bool is_valid() const noexcept
    {
      return h_ != nullptr;
    }

  private:
    void close_if_valid() noexcept
    {
      if (is_valid())
      {
        h_.destroy();
      }
    }
  };

  class [[nodiscard]] co
  {
  public:
    class promise_type
    {
      friend co;

      std::exception_ptr exception_{};

      context* pctx_{ nullptr };
      std::coroutine_handle<> parent_coro_;

    public:
      promise_type() noexcept = default;

      promise_type(const promise_type&) = delete;
      promise_type& operator=(const promise_type&) = delete;

      co get_return_object() noexcept
      {
        return {std::coroutine_handle<promise_type>::from_promise(*this)};
      }

      std::suspend_always initial_suspend() noexcept
      {
        return {};
      }

      std::suspend_always final_suspend() noexcept
      {
        assert(pctx_ != nullptr);
        return {};
      }

      void return_void() noexcept
      {
      }

      void unhandled_exception() noexcept
      {
        assert(nullptr == exception_);
        exception_ = std::current_exception();
      }

      void get_result() const
      {
        if (exception_)
        {
          std::rethrow_exception(exception_);
        }
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        return exception_;
      }

    };

  private:
    class [[nodiscard]] awaiter
    {
      unique_coroutine_handle<promise_type> unique_child_coro_;

    public:
      awaiter(context& ctx, unique_coroutine_handle<promise_type>&& unique_child_coro) noexcept :
        unique_child_coro_{ std::move(unique_child_coro) }
      {
        unique_child_coro_.get().promise().pctx_ = &ctx;
      }

      awaiter(const awaiter&) = delete;
      awaiter& operator=(const awaiter&) = delete;

      [[nodiscard]] constexpr bool await_ready() const noexcept
      {
        return true;
      }

      void await_suspend(std::coroutine_handle<>) noexcept
      {
      }

      void await_resume()
      {
        return;
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        return {};
      }

      void start_as_chain_root() noexcept
      {
      }
    };

    class [[nodiscard]] work
    {
      unique_coroutine_handle<promise_type> unique_child_coro_;

    public:
      work(std::coroutine_handle<promise_type> child_coro) noexcept :
        unique_child_coro_{ child_coro }
      {
      }

      work(const work&) = delete;
      work& operator=(const work&) = delete;
      work(work&&) noexcept = default;
      work& operator=(work&&) noexcept = default;

      [[nodiscard]] awaiter get_awaiter(context& ctx) noexcept
      {
        return {ctx, std::move(unique_child_coro_)};
      }
    };

  private:
    work work_;

    co(std::coroutine_handle<promise_type> child_coro) noexcept :
      work_{ child_coro }
    {
    }

  public:
    co(const co&) = delete;
    co& operator=(const co&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return std::move(work_);
    }
  };
}