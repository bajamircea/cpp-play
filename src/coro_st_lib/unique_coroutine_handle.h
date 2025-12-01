#pragma once

#include "../cpp_util_lib/unique_handle.h"

#include <coroutine>

namespace coro_st
{
  template<typename Promise>
  struct unique_coroutine_handle_traits : cpp_util::unique_handle_basic_access
  {
    using handle_type = std::coroutine_handle<Promise>;
    static constexpr auto invalid_value() noexcept { return nullptr; }
    static constexpr auto is_valid(handle_type h) noexcept { return h.operator bool(); }
    static void close_handle(handle_type h) noexcept { h.destroy(); }
  };
  template<typename Promise>
  using unique_coroutine_handle = cpp_util::unique_handle<unique_coroutine_handle_traits<Promise>>;
}