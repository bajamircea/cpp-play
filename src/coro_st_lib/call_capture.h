#pragma once

#include <tuple>
#include <type_traits>
#include <functional>
#include <utility>

namespace coro_st
{
  template<typename Fn, typename... Args>
  class call_capture
  {
    using ArgsTuple = std::tuple<std::decay_t<Args>...>;

    Fn fn_;
    ArgsTuple args_tuple_;

  public:
    using result_type = decltype(std::apply(fn_, args_tuple_));

    template<typename Fn2, typename... Args2>
    call_capture(Fn2&& fn, Args2&&... args) :
      fn_{ std::forward<Fn2>(fn) },
      args_tuple_{ std::forward<Args2>(args)... }
    {
      static_assert(sizeof... (Args) == sizeof... (Args2));
    }

    result_type operator()()
      noexcept(std::is_nothrow_invocable_v<
        Fn,
        Args...>)
    {
      return std::apply(fn_, args_tuple_);
    }
  };

  template<typename Fn, typename... Args>
  call_capture(Fn&& fn, Args&&... args)
    -> call_capture<Fn, Args...>;
}
