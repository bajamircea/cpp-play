#pragma once

#include "../cpp_util_lib/unique_handle.h"
#include "../cpp_util_lib/handle_arg.h"

#include <cstdio>

namespace cstdio
{
  struct file_handle_traits
  {
    using handle_type = FILE *;
    static constexpr auto invalid_value() noexcept { return nullptr; }
    static void close_handle(handle_type h) noexcept
    {
      static_cast<void>(std::fclose(h));
    }
  };

  using file_handle = cpp_util::unique_handle<file_handle_traits>;
  using file_arg = cpp_util::handle_arg<file_handle_traits>;
}
