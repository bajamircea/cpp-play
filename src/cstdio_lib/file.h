#pragma once

#include "file_raii.h"

#include <cstddef>

namespace cstdio::file
{
  file_raii open(const char * file_name, const char * mode);
  size_t read(file_arg h, char * buffer, size_t size);
  void write(file_arg h, const char * buffer, size_t size);
  bool is_eof(file_arg h);
  void close(file_raii & x);
}
