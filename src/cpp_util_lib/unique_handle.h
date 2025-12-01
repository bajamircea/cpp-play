#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

/*
Basic usage (for more info see unique_handle.md):

struct file_handle_traits : cpp_util::unique_handle_out_ptr_access
{
  using handle_type = FILE*;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(std::fclose(h));
  }
};
using file_handle = cpp_util::unique_handle<file_handle_traits>;
*/

namespace cpp_util
{
  struct unique_handle_basic_access {};
  struct unique_handle_out_ptr_access : unique_handle_basic_access {};
  struct unique_handle_inout_and_out_ptr_access : unique_handle_out_ptr_access {};

  namespace detail
  {
    template<typename Traits>
    concept trait_custom_is_valid = requires(const typename Traits::handle_type const_h)
    {
      { Traits::is_valid(const_h) } noexcept -> std::convertible_to<bool>;
    };
  }

  template<typename Traits>
  concept unique_handle_traits =
    std::derived_from<Traits, unique_handle_basic_access> &&
    requires(
      typename Traits::handle_type h, const typename Traits::handle_type const_h)
    {
      requires std::is_nothrow_copy_constructible_v<typename Traits::handle_type>;
      requires std::is_nothrow_move_constructible_v<typename Traits::handle_type>;
      { Traits::invalid_value() } noexcept;
      { typename Traits::handle_type(Traits::invalid_value()) } noexcept;
      { h = Traits::invalid_value() } noexcept;
      { Traits::close_handle(h) } noexcept;
    };

  template<typename Traits>
  concept unique_handle_custom_is_valid_traits =
    unique_handle_traits<Traits> &&
    requires(const typename Traits::handle_type const_h)
    {
      { Traits::is_valid(const_h) } noexcept -> std::convertible_to<bool>;
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
      requires std::derived_from<Traits, unique_handle_out_ptr_access>
    {
      reset();
      return &h_;
    }

    handle_type* inout_ptr() noexcept
      requires std::derived_from<Traits, unique_handle_inout_and_out_ptr_access>
    {
      return &h_;
    }

    handle_type& inout_ref() noexcept
      requires std::derived_from<Traits, unique_handle_inout_and_out_ptr_access>
    {
      return h_;
    }

    bool is_valid() const noexcept
    {
      if constexpr (unique_handle_custom_is_valid_traits<Traits>)
      {
        return Traits::is_valid(h_);
      }
      else
      {
        static_assert(requires(handle_type h){
          { h == Traits::invalid_value() } noexcept;
          { h != Traits::invalid_value() } noexcept;
        });
        return h_ != Traits::invalid_value();
      }
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