#pragma once

#include <type_traits>
#include <utility>

namespace coro
{
  // TODO: should I use std::move or static_cast<Awaitable&&>?
  template <typename Awaitable>
  concept has_member_operator_co_await =
    requires (Awaitable awaitable) {
      std::move(awaitable).operator co_await();
  };

  template <typename Awaitable>
  concept has_non_member_operator_co_await =
    requires (Awaitable awaitable) {
      operator co_await(std::move(awaitable));
  };

  template <typename Awaitable>
  concept has_get_awaiter =
    requires (Awaitable awaitable) {
      std::move(awaitable).get_awaiter();
  };

  template<typename Awaitable>
  decltype(auto) get_awaiter(Awaitable&& awaitable)
  {
    if constexpr (has_get_awaiter<Awaitable>)
    {
      return std::move(awaitable).get_awaiter();
    }
    else if constexpr (has_member_operator_co_await<Awaitable>)
    {
      return std::move(awaitable).operator co_await();
    }
    else if constexpr (has_non_member_operator_co_await<Awaitable>)
    {
      return operator co_await(std::move(awaitable));
    }
    else
    {
      return std::move(awaitable);
    }
  }
}