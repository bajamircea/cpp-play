#include "../test_lib/test.h"

#include <cstddef>
#include <iterator>

namespace
{
  template<std::forward_iterator I>
  I max_element(I first, I last)
  {
    I return_value = first;
    for(++first; first != last; ++first)
    {
      if (!(*first < *return_value))
      {
        return_value = first;
      }
    }
    return return_value;
  }

  template<std::forward_iterator I, typename T = std::iterator_traits<I>::value_type>
  T average(I first, I last)
  {
    std::size_t count = 0;
    T sum = 0;
    for(; first != last; ++first, ++count)
    {
      sum += *first;
    }
    return sum / count;
  }

  template<std::forward_iterator I, std::forward_iterator O>
  O copy(I first, I last, O result)
  {
    for (;first != last; ++first, ++result)
    {
      *result = *first;
    }
    return result;
  }

  template<std::bidirectional_iterator I>
  void reverse_elements(I first, I last)
  {
    while (first != last)
    {
      --last;
      if (first == last) break;
      std::iter_swap(first, last);
      ++first;
    }
  }

  template<class T, std::size_t N>
  void matrix_multiply(T (&a)[N][N], T (&b)[N][N], T (&c)[N][N])
  {
    for (std::size_t i = 0; i < N; ++i)
    {
      for (std::size_t j = 0; j < N; ++j)
      {
        c[i][j] = a[i][0] * b[0][j];
        for (std::size_t k = 1; k < N; ++k)
        {
          c[i][j] += a[i][k] * b[k][j];
        }
      }
    }
  }

  TEST(p021_max_element)
  {
    int data[] = {3, 42, 12, 42, 11};
    auto max = max_element(std::begin(data), std::end(data));
    ASSERT_EQ(42, *max);
    ASSERT_EQ(3, max - std::begin(data));
  }

  TEST(p021_average)
  {
    double data[] = {3.0, 5.0, 4.0};
    ASSERT_EQ(4.0, average(std::begin(data), std::end(data)));
  }

  TEST(p021_copy)
  {
    int data[] = {1, 2};
    int out[] = {0, 0};
    auto end = copy(std::begin(data), std::end(data), std::begin(out));
    ASSERT_EQ(end, std::end(out));
    ASSERT_EQ(1, out[0]);
    ASSERT_EQ(2, out[1]);
  }

  TEST(p021_reverse_elements)
  {
    int data[] = {1, 2, 3};
    reverse_elements(std::begin(data), std::end(data));
    ASSERT_EQ(3, data[0]);
    ASSERT_EQ(2, data[1]);
    ASSERT_EQ(1, data[2]);
  }

  TEST(p021_matrix_multiply)
  {
    int a[2][2] = {
      {1, 2},
      {2, 3},
    };
    int b[2][2] = {
      {1, 1},
      {2, 3},
    };
    int c[2][2] = {};
    matrix_multiply(a, b, c);
    ASSERT_EQ(5, c[0][0]);
    ASSERT_EQ(7, c[0][1]);
    ASSERT_EQ(8, c[1][0]);
    ASSERT_EQ(11, c[1][1]);
  }
} // anonymous namespace