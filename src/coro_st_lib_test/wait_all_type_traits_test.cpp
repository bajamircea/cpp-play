#include "../test_lib/test.h"

#include "../coro_st_lib/wait_all_type_traits.h"

#include <type_traits>

namespace
{
  TEST(wait_all_type_traits_trivial)
  {
    using namespace coro_st::wait_all_type_traits;
    static_assert(std::is_same_v<
      int,
      value_type_t<int>
    >);
    static_assert(std::is_same_v<
      coro_st::void_result,
      value_type_t<void>
    >);
  }
} // anonymous namespace