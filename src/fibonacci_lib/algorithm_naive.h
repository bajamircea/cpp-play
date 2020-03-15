#pragma once

#include "big_number.h"

#include <cstdint>

namespace fibonacci { namespace algorithm_naive
{
  std::uint64_t exponential_naive(std::uint32_t value);
  std::uint64_t linear_naive(std::uint32_t value);
  fibonacci::big_number::unsigned_binary linear_big(std::uint32_t value);
  std::uint32_t linear_first_big();

}} // namespace fibonacci::algorithm_naive