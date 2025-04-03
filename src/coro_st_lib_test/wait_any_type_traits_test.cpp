#include "../test_lib/test.h"

#include "../coro_st_lib/wait_any_type_traits.h"

namespace
{
  TEST(wait_any_type_traits_trivial)
  {
    using namespace coro_st::wait_any_type_traits;
    static_assert(std::is_same_v<
      int,
      value_type_t<int>
    >);
    static_assert(std::is_same_v<
      int,
      value_type_t<int, short>
    >);
    static_assert(std::is_same_v<
      int,
      value_type_t<int, void>
    >);
    static_assert(std::is_same_v<
      int,
      value_type_t<void, int>
    >);
    static_assert(std::is_same_v<
      void,
      value_type_t<void, void>
    >);
    static_assert(std::is_same_v<
      int,
      value_type_t<void, int, void, short, void>
    >);
  }
} // anonymous namespace