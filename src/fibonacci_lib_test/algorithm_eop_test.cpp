#include "../test_lib/test.h"

#include "../fibonacci_lib/algorithm_eop.h"

#include "../fibonacci_lib/big_number.h"

namespace
{
  using namespace fibonacci::algorithm_eop;
  using namespace fibonacci::big_number;

  TEST(algorithm_sean_parent_fibonacci_pow)
  {
    {
      auto x = fibonacci_pow<unsigned_binary, uint32_t>(5);
      ASSERT_EQ(unsigned_binary(5), x);
    }
    {
      auto x = fibonacci_pow<unsigned_binary, uint32_t>(44);
      ASSERT_EQ(unsigned_binary(701408733), x);
    }
  }
} // anonymous namespace