#pragma once
namespace cpp_util
{
  template<typename RaiiClass>
  class handle_arg
  {
    using handle = typename RaiiClass::handle;

    handle h_;

  public:
    handle_arg(handle h) noexcept :
      h_{ h }
    {
    }

    handle_arg(const RaiiClass & raii) noexcept :
      h_{ raii.get() }
    {
    }

    handle_arg(RaiiClass &&) noexcept = delete;

    operator handle() const noexcept
    {
      return h_;
    }
  };
}