#pragma once

namespace cstdio::error
{
  void throw_errno(const char * function_name, int errno_value);
  void throw_errno(const char * function_name);
  void throw_failed(const char * function_name);
}
