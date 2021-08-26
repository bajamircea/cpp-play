#include "../test_lib/test.h"

#include <cstddef>
#include <iterator>

namespace
{
  template <typename T>
  void insertion_sort(T* a, std::size_t n)
  {
    for (std::size_t j = 1; j < n; ++j)
    {
      T key = a[j];
      std::size_t i = j;
      while ((i > 0) && (a[i - 1] > key))
      {
        a[i] = a[i - 1];
        --i;
      }
      a[i] = key;
    }
  }

  template <typename T>
  void assert_is_sorted(const T * a, std::size_t n)
  {
    for (std::size_t i = 1; i < n; ++i)
    {
      ASSERT(a[i - 1] <= a[i]);
    }
  }

  TEST(ch_02_01_insertion_sort_simple)
  {
    int data[] = {31, 41, 59, 26, 41, 58};
    insertion_sort(data, std::size(data));
    assert_is_sorted(data, std::size(data));
  }
} // anonymous namespace