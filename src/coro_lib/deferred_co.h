#pragma once

#include <tuple>
#include <type_traits>
#include <functional>
#include <utility>

namespace coro
{
  template<typename CoFn, typename... CapturedArgs>
  class [[nodiscard]] deferred_co_fn
  {
    using CoFnDecayed = std::decay_t<CoFn>;
    using CapturedArgsTuple = std::tuple<std::decay_t<CapturedArgs>...>;
    using CapturedArgsSeq = std::index_sequence_for<CapturedArgs...>;

    CoFnDecayed co_fn_;
    CapturedArgsTuple captured_args_tuple_;

  public:
    explicit deferred_co_fn(CoFn&& co_fn, CapturedArgs&&... captured_args) :
      co_fn_{ std::forward<CoFn>(co_fn) },
      captured_args_tuple_{ std::forward<CapturedArgs>(captured_args)... }
    {
    }

    template<typename... ExtraCallArgs>
    auto operator()(ExtraCallArgs&&... extra_call_args)
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

  // TODO: could I eliminate this and use deduction rules instead?
  template<typename CoFn, typename... CapturedArgs>
  auto deferred_co(CoFn&& co_fn, CapturedArgs&&... captured_args)
  {
    return deferred_co_fn<CoFn, CapturedArgs...>(
      std::forward<CoFn>(co_fn),
      std::forward<CapturedArgs>(captured_args)...);
  }
}
