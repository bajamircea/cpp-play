#include "algorithm.h"

namespace fibonacci { namespace algorithm
{
  std::uint64_t exponential_naive(std::uint32_t value)
  {
    if (value < 2)
    {
      return 1;
    }
    return exponential_naive(value - 1) + exponential_naive(value - 2);
  }

  std::uint64_t linear_naive(std::uint32_t value)
  {
    if (value < 2)
    {
      return 1;
    }
    std::uint64_t a = 1;
    std::uint64_t b = 1;
    for (value -= 1; value > 0; --value)
    {
      std::uint64_t c = a + b;
      b = a;
      a = c;
    }
    return a;
  }

  fibonacci::big_number::unsigned_binary linear_big(std::uint32_t value)
  {
    if (value < 2)
    {
      return fibonacci::big_number::unsigned_binary(1);
    }
    fibonacci::big_number::unsigned_binary a(1);
    fibonacci::big_number::unsigned_binary b(1);
    for (value -= 1; value > 0; --value)
    {
      b += a;
      std::swap(a.units_, b.units_);
    }
    return a;
  }

  std::uint32_t linear_first_big()
  {
    fibonacci::big_number::unsigned_binary a(1);
    fibonacci::big_number::unsigned_binary b(1);
    for (std::uint32_t return_value = 2; ; ++return_value)
    {
      b += a;
      std::swap(a.units_, b.units_);
      if (a.units_.size() > 2)
      {
        return return_value;
      }
    }
  }
}} // namespace fibonacci::algorithm