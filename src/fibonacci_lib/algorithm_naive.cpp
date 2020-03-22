#include "algorithm_naive.h"

namespace fibonacci { namespace algorithm_naive
{
  std::uint64_t exponential_naive(std::uint32_t x)
  {
    if (x < 2)
    {
      return x;
    }
    return exponential_naive(x - 1) + exponential_naive(x - 2);
  }

  std::uint64_t linear_naive(std::uint32_t x)
  {
    if (x < 2)
    {
      return x;
    }
    std::uint64_t a = 0;
    std::uint64_t b = 1;
    for (--x; x > 0; --x)
    {
      std::uint64_t c = a + b;
      a = b;
      b = c;
    }
    return b;
  }

  fibonacci::big_number::unsigned_binary linear_big(std::uint32_t x)
  {
    if (x < 2)
    {
      return fibonacci::big_number::unsigned_binary(x);
    }
    fibonacci::big_number::unsigned_binary a(0);
    fibonacci::big_number::unsigned_binary b(1);
    for (--x; x > 0; --x)
    {
      a += b;
      std::swap(a.units_, b.units_);
    }
    return b;
  }

  std::uint32_t linear_first_over_uint64_t()
  {
    fibonacci::big_number::unsigned_binary a(0);
    fibonacci::big_number::unsigned_binary b(1);
    for (std::uint32_t return_value = 2; ; ++return_value)
    {
      a += b;
      if (a.units_.size() > 2)
      {
        return return_value;
      }
      std::swap(a.units_, b.units_);
    }
  }
}} // namespace fibonacci::algorithm_naive