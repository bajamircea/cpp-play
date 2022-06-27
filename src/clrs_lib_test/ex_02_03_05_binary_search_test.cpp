#include "../test_lib/test.h"

#include <cstddef>
#include <iterator>
#include <vector>

namespace
{
  template <typename T>
  std::size_t binary_search(const T * a, std::size_t n, const T& value)
  {
    std::size_t not_found = n;
    const T * p = a;
    while (n != 0)
    {
      std::size_t m = n / 2;
      if (value < p[m])
      {
        n = m;
      }
      else if (p[m] < value)
      {
        p += m + 1;
        n -= m + 1;
      }
      else
      {
        return p + m - a;
      }
    }
    return not_found;
  }

  TEST(ex_02_03_05_binary_search_test_simple)
  {
    int data[] = {26, 31, 40, 42, 58, 60};
    std::size_t len = std::size(data);
    for (size_t j = 1; j <= len; ++j)
    {
      for (size_t i = 0; i < j; ++i)
      {
        ASSERT_EQ(j, binary_search(data, j, data[i]-1));
        ASSERT_EQ(i, binary_search(data, j, data[i]));
        ASSERT_EQ(j, binary_search(data, j, data[i]+1));
      }
    }
    ASSERT_EQ(0, binary_search(data, 0, 42));
  }
} // anonymous namespace