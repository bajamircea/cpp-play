#pragma once

#include <tuple>
#include <type_traits>
#include <functional>
#include <utility>

namespace coro
{
  template<typename Ret, typename CoFn, typename... Args>
  class [[nodiscard]] deferred_co_fn
  {
    using CoFnDecayed = std::decay_t<CoFn>;
    using ArgsDecayedTuple = std::tuple<std::decay_t<Args>...>;
    using ArgsSequence = std::index_sequence_for<Args...>;

    CoFnDecayed co_fn_;
    ArgsDecayedTuple args_;

  public:
    using co_return_type = Ret;

    explicit deferred_co_fn(CoFn&& co_fn, Args&&... args) :
      co_fn_{ std::forward<CoFn>(co_fn) },
      args_{ std::forward<Args>(args)... }
    {
    }

    template<typename... ExtraArgs>
    auto operator()(ExtraArgs&&... extraArgs)
    {
      return invoke_impl(ArgsSequence{}, std::forward<ExtraArgs>(extraArgs)...);
    }

  private:
    template<std::size_t... I, typename... ExtraArgs>
    auto invoke_impl(std::index_sequence<I...>, ExtraArgs&&... extraArgs)
    {
      return std::invoke(co_fn_, std::forward<ExtraArgs>(extraArgs)..., std::get<I>(args_)...);
    }
  };

  template<typename Ret, typename CoFn, typename... Args>
  auto deferred_co(CoFn&& co_fn, Args&&... args)
  {
    return deferred_co_fn<Ret, CoFn, Args...>(std::forward<CoFn>(co_fn), std::forward<Args>(args)...);
  }
}
