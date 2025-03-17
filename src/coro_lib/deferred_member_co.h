#pragma once

#include <tuple>
#include <type_traits>
#include <functional>
#include <utility>

namespace coro
{
  template<typename CoFn, typename Self, typename... CapturedArgs>
  class [[nodiscard]] deferred_member_co
  {
    using CoFnDecayed = std::decay_t<CoFn>;
    using CapturedArgsTuple = std::tuple<std::decay_t<CapturedArgs>...>;
    using CapturedArgsSeq = std::index_sequence_for<CapturedArgs...>;

    CoFnDecayed co_fn_;
    Self* self_;
    CapturedArgsTuple captured_args_tuple_;

  public:
    template<typename CoFn2, typename Self2, typename... CapturedArgs2>
    deferred_member_co(CoFn2&& co_fn, Self2* self,CapturedArgs2&&... captured_args) :
      co_fn_{ std::forward<CoFn2>(co_fn) },
      self_{ self },
      captured_args_tuple_{ std::forward<CapturedArgs2>(captured_args)... }
    {
    }

    template<typename... ExtraCallArgs>
    auto operator()(ExtraCallArgs&&... extra_call_args)
      noexcept(std::is_nothrow_invocable_v<
        CoFnDecayed,
        Self*,
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
        self_,
        std::forward<ExtraCallArgs>(extra_call_args)...,
        std::get<I>(captured_args_tuple_)...);
    }
  };

  template<typename CoFn, typename Self, typename... CapturedArgs>
  deferred_member_co(CoFn&& co_fn, Self* self, CapturedArgs&&... captured_args)
    -> deferred_member_co<CoFn, Self, CapturedArgs...>;
}
