#include "../test_lib/test.h"

#include <cstddef>
#include <iterator>

namespace
{
  template<std::random_access_iterator I, typename T = std::iterator_traits<I>::value_type>
  I lower_bound(I first, I last, const T & value)
  {
    // if the size is 0, we're done
    while(first != last)
    {
      // if the size is 2k (k > 0)
      //   half_length is k
      //   first range length is k
      //   first range length is k (becomes k - 1)
      // else if the size is 2k+1 (k>=0)
      //   half_length is k
      //   first range length is k
      //   first range length is k + 1 (becomes k)
      std::size_t half_length = (last - first) / 2;
      I mid = first + half_length;
      if (*mid < value)
      {
        ++first;
      }
      else
      {
        last = mid;
      }
    }
    return first;
  }

  template<std::random_access_iterator I, typename T = std::iterator_traits<I>::value_type>
  I rank(I first, I last, const T & value)
  {
    I it = lower_bound(first, last, value);
    if ((it != last) && (value < *it))
    {
      return last;
    }
    return it;
  }

  // p025
  template<std::random_access_iterator I, typename T = std::iterator_traits<I>::value_type>
  I lower_bound_recursive(I first, I last, const T & value)
  {
    if(first == last)
    {
      return first;
    }
    {
      std::size_t half_length = (last - first) / 2;
      I mid = first + half_length;
      if (*mid < value)
      {
        ++first;
      }
      else
      {
        last = mid;
      }
    }
    return lower_bound_recursive(first, last, value);
  }

  TEST(p009_rank)
  {
    int data[] = {3, 42, 52, 74, 110};

    auto fourty_two = lower_bound(std::begin(data), std::end(data), 42);
    ASSERT_EQ(42, *fourty_two);
    ASSERT_EQ(fourty_two, rank(std::begin(data), std::end(data), 42));
    ASSERT_EQ(fourty_two, lower_bound(std::begin(data), std::end(data), 42));

    auto fourty_three = lower_bound(std::begin(data), std::end(data), 43);
    ASSERT_EQ(52, *fourty_three);
    ASSERT_EQ(std::end(data), rank(std::begin(data), std::end(data), 43));
    ASSERT_EQ(fourty_three, lower_bound(std::begin(data), std::end(data), 43));


    auto two_cents = lower_bound(std::begin(data), std::end(data), 200);
    ASSERT_EQ(std::end(data), two_cents);
    ASSERT_EQ(std::end(data), rank(std::begin(data), std::end(data), 200));
    ASSERT_EQ(two_cents, lower_bound(std::begin(data), std::end(data), 200));
  }
} // anonymous namespace