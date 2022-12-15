#pragma once
namespace cpp_util
{
  template<typename Traits>
  // requires Traits defines
  // - a handle type - the type of the handle
  // - a function static void close_handle(handle) noexcept - the function to
  //   close the handle
  // - and an invalid_value constant - the special value indicating "no handle"
  class raii_with_invalid_value
  {
  public:
    using handle = typename Traits::handle;

  private:
    handle h_;

  public:
    raii_with_invalid_value() noexcept :
      h_{ Traits::invalid_value }
    {
    }

    explicit raii_with_invalid_value(handle h) noexcept :
      h_{ h }
    {
    }

    ~raii_with_invalid_value()
    {
      if (is_valid())
      {
        Traits::close_handle(h_);
      }
    }

    raii_with_invalid_value(const raii_with_invalid_value &) = delete;
    raii_with_invalid_value & operator=(const raii_with_invalid_value &) = delete;

    raii_with_invalid_value(raii_with_invalid_value && other) noexcept :
      h_{ other.h_ }
    {
      other.h_ = Traits::invalid_value;
    }

    raii_with_invalid_value & operator=(raii_with_invalid_value && other) noexcept
    {
      handle tmp = other.h_;
      other.h_ = Traits::invalid_value;
      if (is_valid())
      {
        Traits::close_handle(h_);
      }
      h_ = tmp;
      return *this;
    }

    bool is_valid() const noexcept
    {
      return h_ != Traits::invalid_value;
    }

    explicit operator bool() const noexcept
    {
      return is_valid();
    }

    handle get() const noexcept
    {
      return h_;
    }

    handle & handle_reference() noexcept
    {
      return h_;
    }

    handle * handle_pointer() noexcept
    {
      return &h_;
    }

    void reset(handle h = Traits::invalid_value) noexcept
    {
      if (is_valid())
      {
        Traits::close_handle(h_);
      }
      h_ = h;
    }

    handle release() noexcept
    {
      handle tmp = h_;
      h_ = Traits::invalid_value;
      return tmp;
    }
  };
}