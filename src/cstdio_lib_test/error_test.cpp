#include "../test_lib/test.h"

#include "../cstdio_lib/error.h"

#include <stdexcept>

namespace
{
  TEST(throw_errno_simple)
  {
    ASSERT_THROW_WHAT((cstdio::error::throw_errno("foo", 2)), std::runtime_error, "Function foo failed. Error: No such file or directory");
  }

  TEST(throw_failed_simple)
  {
    ASSERT_THROW_WHAT(cstdio::error::throw_failed("foo"), std::runtime_error, "Function foo failed");
  }
} // anonymous namespace