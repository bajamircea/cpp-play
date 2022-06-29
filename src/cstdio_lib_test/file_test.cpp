#include "../test_lib/test.h"

#include "../cstdio_lib/file.h"

#include <filesystem>
#include <iostream>

namespace
{
  TEST(file_read_simple)
  {
    auto self_path = std::filesystem::read_symlink("/proc/self/exe");
    auto file = cstdio::file::open(self_path.c_str(), "rb");
    char buffer[2];
    ASSERT_EQ(2, cstdio::file::read(file, buffer, std::size(buffer)));
    ASSERT_FALSE(cstdio::file::is_eof(file));
    cstdio::file::close(file);
  }
} // anonymous namespace