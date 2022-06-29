#include "error.h"

#include <cstring>
#include <stdexcept>
#include <string>

namespace cstdio::error
{
  void throw_errno(const char * function_name, int errno_value)
  {
    std::string message = "Function ";
    message += function_name;
    message += " failed. Error: ";
    message += std::strerror(errno_value);

    throw std::runtime_error(message);
  }

  void throw_errno(const char * function_name)
  {
    throw_errno(function_name, errno);
  }

  void throw_failed(const char * function_name)
  {
    std::string message = "Function ";
    message += function_name;
    message += " failed";

    throw std::runtime_error(message);
  }
}