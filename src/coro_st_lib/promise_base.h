#pragma once

#include <cassert>
#include <exception>

namespace coro_st
{
  template<typename T>
  class promise_base
  {
    std::exception_ptr exception_{};

  public:
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
}
