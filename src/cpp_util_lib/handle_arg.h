#pragma once

#include <utility>

namespace cpp_util
{
  // Adapter class that allows defining function arguments that accept
  // either a caller-owned raw handle or a RAII unique_handle.
  // The constructors are intentionally not explicit.
  template<typename UniqueHandle>
  struct handle_arg
  {
    using handle_type = typename UniqueHandle::handle_type;

    handle_type h;

    handle_arg(handle_type h) noexcept :
      h{ std::move(h) }
    {
    }

    handle_arg(const UniqueHandle & h) noexcept :
      h{ h.get() }
    {
    }

    // Make it harder to have dangling references,
    // but looses convenience of single line `read(open(...), ...);`
    handle_arg(UniqueHandle &&) noexcept = delete;

    operator handle_type() const noexcept
    {
      return h;
    }
  };
}