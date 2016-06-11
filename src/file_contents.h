#pragma once

#include <string>

namespace file_contents
{
  std::string read(const std::string & file_name);
  void write(const std::string & file_name, const std::string & contents);
} // namespace file_contents
