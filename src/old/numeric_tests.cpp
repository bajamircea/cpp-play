#include "numeric.h"

#include "test.h"

namespace
{
  using namespace numeric;

  TEST(numeric__cast_long_to_unsigned_long)
  {
    ASSERT_EQ(42UL, cast<unsigned long>(42L));
    ASSERT_THROW(cast<unsigned long>(-1L), std::bad_cast);
  }

  TEST(numeric__cast_unsigned_long_to_long)
  {
    ASSERT_EQ(42L, cast<long>(42UL));
    ASSERT_THROW(cast<long>(static_cast<unsigned long>(-1L)), std::bad_cast);
  }
} // anonymous namespace
