#include "file_contents.h"

#include "test.h"

#include <stdexcept>

namespace
{
  using namespace file_contents;

  TEST(file_contents__read_write)
  {
    std::string contents{ "abc\r\ndef" };
    std::string file_name{ "tmp/test/file_contents__read_write__simple" };

    write(file_name, contents);

    std::string actual{ read(file_name) };

    ASSERT_EQ(contents, actual);
  }

  TEST(file_contents__read_missing)
  {
    ASSERT_THROW(read("tmp/test/file_contents__read_missing__missing"), std::runtime_error);
  }
} // anonymous namespace
