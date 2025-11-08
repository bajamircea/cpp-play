#pragma once

#include <utility>
#include <coroutine>

namespace cpp_util
{
  template<typename Traits>
  class [[nodiscard]] unique_handle
  {
  public:
    using traits_type = Traits;
    using handle_type = typename Traits::handle_type;

  private:
    handle_type h_;

  public:
    unique_handle() noexcept :
      h_{ Traits::invalid_value() }
    {
    }

    explicit unique_handle(const handle_type& h) noexcept :
      h_{ h }
    {
    }

    explicit unique_handle(handle_type&& h) noexcept :
      h_{ std::move(h) }
    {
    }

    template<typename Arg1, typename Arg2, typename ... Args>
      requires(std::is_nothrow_constructible_v<handle_type, Arg1, Arg2, Args...>)
    unique_handle(Arg1 && arg1, Arg2 && arg2, Args && ... args) noexcept
      :
      h_{ std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Args>(args) ... }
    {
    }

    ~unique_handle()
    {
      close_if_valid();
    }

    unique_handle(const unique_handle &) = delete;
    unique_handle & operator=(const unique_handle &) = delete;

    unique_handle(unique_handle && other) noexcept :
      h_{ std::move(other.h_) }
    {
      other.h_ = Traits::invalid_value();
    }

    unique_handle & operator=(unique_handle && other) noexcept
    {
      handle_type tmp = std::move(other.h_);
      other.h_ = Traits::invalid_value();
      close_if_valid();
      h_ = std::move(tmp);
      return *this;
    }

    handle_type get() const noexcept
    {
      return h_;
    }

    handle_type * handle_pointer() noexcept
    {
      return &h_;
    }

    handle_type & ref() noexcept
    {
      return h_;
    }

    bool is_valid() const noexcept
    {
      return h_ != Traits::invalid_value();
    }

    explicit operator bool() const noexcept
    {
      return is_valid();
    }

    void reset(handle_type h = Traits::invalid_value()) noexcept
    {
      close_if_valid();
      h_ = std::move(h);
    }

    [[nodiscard]]
    handle_type release() noexcept
    {
      handle_type tmp = std::move(h_);
      h_ = Traits::invalid_value();
      return tmp;
    }

  private:
    void close_if_valid() noexcept
    {
      if (is_valid())
      {
        Traits::close_handle(h_);
      }
    }
  };
}

namespace coro_st
{
  template<typename Promise>
  struct unique_coroutine_handle_traits
  {
    using handle_type = std::coroutine_handle<Promise>;
    static constexpr auto invalid_value() noexcept { return nullptr; }
    static constexpr auto is_valid(handle_type h) noexcept { return h.operator bool(); }
    static void close_handle(handle_type h) noexcept { h.destroy(); }
  };
  template<typename Promise>
  using unique_coroutine_handle = cpp_util::unique_handle<unique_coroutine_handle_traits<Promise>>;
}