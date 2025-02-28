#pragma once

#include <tuple>
#include <type_traits>

namespace coro
{
  template<typename CoFn, typename... Args>
  class [[nodiscard]] deferred_co_fn
  {
    using CoFnDecayed = std::decay_t<CoFn>;
    using ArgsDecayedTuple = std::tuple<std::decay_t<Args>...>;
    using InvokeResult = std::invoke_result_t<CoFnDecayed, std::decay_t<Args>...>;

    CoFnDecayed co_fn_;
    ArgsDecayedTuple args_;

  public:
    using co_return_type = InvokeResult::co_return_type;

    explicit deferred_co_fn(CoFn&& co_fn, Args&&... args) :
      co_fn_{ std::forward<CoFn>(co_fn) },
      args_{ std::forward<Args>(args)... }
    {
    }

    InvokeResult operator()()
    {
      co_return co_await std::apply(co_fn_, args_);
    }
  };

  template<typename CoFn, typename... Args>
  auto deferred_co(CoFn&& co_fn, Args&&... args)
  {
    return deferred_co_fn<CoFn, Args...>(std::forward<CoFn>(co_fn), std::forward<Args>(args)...);
  }
}
