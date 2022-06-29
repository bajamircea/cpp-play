#pragma once

#include "../cpp_util_lib/raii_with_invalid_value.h"
#include "../cpp_util_lib/handle_arg.h"

#include <cstdio>

namespace cstdio
{
  namespace detail
  {
    struct file_raii_traits
    {
      using handle = FILE *;

      static constexpr auto invalid_value = nullptr;

      static void close_handle(handle h) noexcept
      {
        static_cast<void>(fclose(h));
      }
    };
  }

  using file_raii = cpp_util::raii_with_invalid_value<detail::file_raii_traits>;
  using file_arg = cpp_util::handle_arg<file_raii>;
}
