#include "../test_lib/test.h"

#include <cstddef>
#include <iterator>
#include <vector>

namespace
{
  template <typename T>
  void merge(T* a, std::size_t p, std::size_t q, std::size_t r, T* buff)
  {
    size_t p2 = 0;
    size_t q2 = q - p;
    for (size_t i = 0; i < q2; ++i)
    {
      buff[i] = a[p + i];
    }
    size_t j = p;
    for (;(p2 < q2) && (q < r); ++j)
    {
      if (a[q] < buff[p2])
      {
        a[j] = a[q];
        ++q;
      }
      else
      {
        a[j] = buff[p2];
        ++p2;
      }
    }
    for (;p2 < q2; ++j, ++p2)
    {
      a[j] = buff[p2];
    }
  }

  template <typename T>
  void merge_sort_impl(T* a, std::size_t p, std::size_t r, T* buff)
  {
    if (p + 1 < r)
    {
      std::size_t q = p + (r - p) / 2;
      merge_sort_impl(a, p, q, buff);
      merge_sort_impl(a, q, r, buff);
      merge(a, p, q, r, buff);
    }
  }

  template <typename T>
  void merge_sort(T* a, std::size_t n)
  {
    if (n < 2)
    {
      return;
    }
    std::vector<T> buff(n/2);
    merge_sort_impl(a, 0, n, buff.data());
  }

  template <typename T>
  void assert_is_sorted(const T * a, std::size_t n)
  {
    for (std::size_t i = 1; i < n; ++i)
    {
      ASSERT(a[i - 1] <= a[i]);
    }
  }

  template <typename T>
  void assert_array_equals(const T * a, std::size_t n, const T * b)
  {
    for (std::size_t i = 0; i < n; ++i)
    {
      ASSERT_EQ(a[i], b[i]);
    }
  }

  TEST(ch_02_03_01_merge_sort_simple)
  {
    int data[] = {31, 41, 59, 26, 41, 58};
    merge_sort(data, std::size(data));
    assert_is_sorted(data, std::size(data));
    int expected_data[] = {26, 31, 41, 41, 58, 59};
    assert_array_equals(data, std::size(data), expected_data);
  }

  TEST(ch_02_03_01_merge_sort_odd_len)
  {
    int data[] = {31, 41, 59, 26, 41};
    merge_sort(data, std::size(data));
    assert_is_sorted(data, std::size(data));
    int expected_data[] = {26, 31, 41, 41, 59};
    assert_array_equals(data, std::size(data), expected_data);
  }
} // anonymous namespace