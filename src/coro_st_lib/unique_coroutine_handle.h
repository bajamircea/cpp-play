#pragma once

#include <utility>
#include <coroutine>

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
}