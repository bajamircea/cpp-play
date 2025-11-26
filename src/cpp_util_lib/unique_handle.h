#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

namespace cpp_util
{
  // see unique_handle.md

  template<typename Traits>
  concept unique_handle_traits = requires(
    typename Traits::handle_type h, const typename Traits::handle_type ch)
  {
    { Traits::invalid_value() } noexcept;
    { typename Traits::handle_type(Traits::invalid_value()) } noexcept;
    { h = Traits::invalid_value() } noexcept;
    { Traits::close_handle(ch) } noexcept;
    { typename Traits::handle_type(std::move(h)) } noexcept;
    { typename Traits::handle_type(ch) } noexcept;
  };

  template<typename Traits>
  concept unique_handle_custom_is_valid_traits =
    unique_handle_traits<Traits> &&
    requires(const typename Traits::handle_type ch)
  {
    { Traits::is_valid(ch) } noexcept -> std::convertible_to<bool>;
  };

  template<unique_handle_traits Traits>
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
    unique_handle(Arg1&& arg1, Arg2&& arg2, Args&& ... args) noexcept
      :
      h_{ std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Args>(args) ... }
    {
    }

    ~unique_handle()
    {
      close_if_valid();
    }

    unique_handle(const unique_handle&) = delete;
    unique_handle& operator=(const unique_handle&) = delete;

    unique_handle(unique_handle&& other) noexcept :
      h_{ std::move(other.h_) }
    {
      other.h_ = Traits::invalid_value();
    }

    unique_handle& operator=(unique_handle&& other) noexcept
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

    handle_type* out_ptr() noexcept
    {
      reset();
      return &h_;
    }

    handle_type* inout_ptr() noexcept
    {
      return &h_;
    }

    handle_type& inout_ref() noexcept
    {
      return h_;
    }

    bool is_valid() const noexcept
    {
      if constexpr (unique_handle_custom_is_valid_traits<Traits>)
        return Traits::is_valid(h_);
      else
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