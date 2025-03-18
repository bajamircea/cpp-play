#include "../test_lib/test.h"

#include <limits>
#include <iterator>

namespace
{
  struct max_sum_subarray_result
  {
    const int * first;
    const int * last;
    int sum;
  };

  max_sum_subarray_result max_sum_subarray(const int * first, const int * last)
  {
    max_sum_subarray_result result{ .first=first, .last=last, .sum=std::numeric_limits<int>::min() };
    if (first == last)
    {
      return result;
    }
    const int * crt_first = first;
    int crt_sum = *first;
    result.sum = crt_sum;
    ++first;

    for (;first != last; ++first)
    {
      int val = *first;
      if (crt_sum < 0)
      {
        crt_first = first;
        crt_sum = val;
      }
      else
      {
        crt_sum += val;
      }
      if (crt_sum > result.sum)
      {
        result.first = crt_first;
        result.last = first;
        result.sum = crt_sum;
      }
    }
    ++(result.last);
    return result;
  }

  void fill_change(int * price_first, int * price_last, int * change)
  {
    if (price_first == price_last)
    {
      return;
    }
    int previous = *price_first;
    ++price_first;
    for (; price_first != price_last; ++price_first, ++change)
    {
      *change = *price_first - previous;
      previous = *price_first;
    }
  }

  TEST(ex_04_01_05_max_sum_subarray_simple)
  {
    int price[] = {100, 113, 110, 85, 105, 102, 86, 63, 81, 101, 94, 106, 101, 79, 94, 90, 97};
    int change[std::size(price) - 1];
    fill_change(std::begin(price), std::end(price), change); 
    auto result = max_sum_subarray(std::begin(change), std::end(change));
    // buy end of day 7, sell end of day 11, make 106 - 63 = 43
    ASSERT_EQ(43, result.sum);
    ASSERT_EQ(7, result.first - std::begin(change));
    ASSERT_EQ(11, result.last - std::begin(change));
  }
} // anonymous namespace