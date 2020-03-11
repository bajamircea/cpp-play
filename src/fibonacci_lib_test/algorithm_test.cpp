#include "../test_lib/test.h"

#include "../fibonacci_lib/algorithm.h"

namespace
{
  using namespace fibonacci::algorithm;
  using namespace fibonacci::big_number;

  TEST(algorithm_exponential_naive)
  {
    ASSERT_EQ(2, exponential_naive(2));
    ASSERT_EQ(3, exponential_naive(3));
    ASSERT_EQ(5, exponential_naive(4));
  }

  TEST(algorithm_linear_naive)
  {
    ASSERT_EQ(2, linear_naive(2));
    ASSERT_EQ(3, linear_naive(3));
    ASSERT_EQ(5, linear_naive(4));
  }

  TEST(algorithm_linear_big)
  {
    ASSERT_EQ(unsigned_binary(5), linear_big(4));
  }

  TEST(algorithm_fibonacci_pow)
  {
    auto x = fibonacci_pow<unsigned_binary, uint32_t>(5);
    ASSERT_EQ(unsigned_binary(5), x);
  }
} // anonymous namespace