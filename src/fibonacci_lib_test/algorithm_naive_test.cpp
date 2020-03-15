#include "../test_lib/test.h"

#include "../fibonacci_lib/algorithm_naive.h"

namespace
{
  using namespace fibonacci::algorithm_naive;
  using namespace fibonacci::big_number;

  TEST(algorithm_naive_exponential_naive)
  {
    ASSERT_EQ(2, exponential_naive(2));
    ASSERT_EQ(3, exponential_naive(3));
    ASSERT_EQ(5, exponential_naive(4));
  }

  TEST(algorithm_naive_linear_naive)
  {
    ASSERT_EQ(2, linear_naive(2));
    ASSERT_EQ(3, linear_naive(3));
    ASSERT_EQ(5, linear_naive(4));
  }

  TEST(algorithm_naive_linear_big)
  {
    ASSERT_EQ(unsigned_binary(5), linear_big(4));
  }
} // anonymous namespace