#pragma once

#include <cassert>
#include <concepts>
#include <exception>
#include <type_traits>
#include <utility>
#include <variant>

namespace coro_st
{
  template<typename T>
  class promise_base
  {
    std::variant<std::monostate, T, std::exception_ptr> result_;

  public:
    template<typename U>
      requires std::convertible_to<U, T>
    void return_value(U && x) noexcept(std::is_nothrow_constructible_v<T, U>)
    {
      assert(0 == result_.index());
      result_.template emplace<1>(std::forward<U>(x));
    }

    void unhandled_exception() noexcept
    {
      assert(0 == result_.index());
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

    std::exception_ptr get_result_exception() const noexcept
    {
      if (2 != result_.index())
      {
        return {};
      }
      return std::get<2>(result_);
    }
  };

  template<>
  class promise_base<void>
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
