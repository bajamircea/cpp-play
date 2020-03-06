#include "../test_lib/test.h"

#include "../fibonacci_lib/hello.h"

namespace
{
  TEST(fib_lib_dummy)
  {
    ASSERT_EQ(0, fibonacci::hello("ahaaa"));
  }
} // anonymous namespace