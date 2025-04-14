#pragma once

#include "unique_handle.h"

#include <utility>

namespace cpp_util
{
  // Adapter class that allows defining function arguments that accept
  // either a caller-owned raw handle or a RAII unique_handle.
  // The constructors are intentionally not explicit.
  template<unique_handle_traits Traits>
  struct handle_arg
  {
    using handle_type = typename Traits::handle_type;
    using unique_handle_type = unique_handle<Traits>;

    handle_type h;

    handle_arg(handle_type h) noexcept :
      h{ std::move(h) }
    {
    }

    handle_arg(const unique_handle_type & h) noexcept :
      h{ h.get() }
    {
    }

    // Make it harder to have dangling references,
    // but looses convenience of single line `read(open(...), ...);`
    handle_arg(unique_handle_type &&) noexcept = delete;

    operator handle_type() const noexcept
    {
      return h;
    }
  };
}