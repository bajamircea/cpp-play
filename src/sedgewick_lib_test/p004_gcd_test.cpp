#include "../test_lib/test.h"

namespace
{
  int gcd(int p, int q)
  {
    if (q == 0)
    {
      return p;
    }
    int r = p % q;
    return gcd(q, r);
  }

  TEST(p004_gcd_somple)
  {
    ASSERT_EQ(8, gcd(32, 24));
  }
} // anonymous namespace