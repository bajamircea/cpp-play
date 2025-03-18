#pragma once

#include <tuple>
#include <type_traits>
#include <functional>
#include <utility>

namespace coro
{
  template<typename CoFn, typename... CapturedArgs>
  class [[nodiscard]] deferred_co
  {
    // TODO: why decay here?
    using CoFnDecayed = std::decay_t<CoFn>;
    using CapturedArgsTuple = std::tuple<std::decay_t<CapturedArgs>...>;
    using CapturedArgsSeq = std::index_sequence_for<CapturedArgs...>;

    CoFnDecayed co_fn_;
    CapturedArgsTuple captured_args_tuple_;

  public:
    template<typename CoFn2, typename... CapturedArgs2>
    deferred_co(CoFn2&& co_fn, CapturedArgs2&&... captured_args) :
      co_fn_{ std::forward<CoFn2>(co_fn) },
      captured_args_tuple_{ std::forward<CapturedArgs2>(captured_args)... }
    {
      static_assert(sizeof... (CapturedArgs) == sizeof... (CapturedArgs2));
    }

    template<typename... ExtraCallArgs>
    auto operator()(ExtraCallArgs&&... extra_call_args)
      noexcept(std::is_nothrow_invocable_v<
        CoFnDecayed,
        ExtraCallArgs...,
        CapturedArgs...>)
    {
      return invoke_impl(
        CapturedArgsSeq{},
        std::forward<ExtraCallArgs>(extra_call_args)...);
    }

  private:
    template<std::size_t... I, typename... ExtraCallArgs>
    auto invoke_impl(std::index_sequence<I...>, ExtraCallArgs&&... extra_call_args)
    {
      return std::invoke(
        co_fn_,
        std::forward<ExtraCallArgs>(extra_call_args)...,
        std::get<I>(captured_args_tuple_)...);
    }
  };

  template<typename CoFn, typename... CapturedArgs>
  deferred_co(CoFn&& co_fn, CapturedArgs&&... captured_args)
    -> deferred_co<CoFn, CapturedArgs...>;
}
