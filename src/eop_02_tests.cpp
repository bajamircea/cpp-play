#include "eop_02.h"

#include "test.h"

namespace
{
  using namespace eop_02;

  unsigned int incr(unsigned int value)
  {
    return (value + 1);
  }

  TEST(eop_02__power_unary)
  {
    ASSERT_EQ(1, power_unary(1u, 0, incr));
    ASSERT_EQ(2, power_unary(1u, 1, incr));
    ASSERT_EQ(5, power_unary(1u, 4, incr));
  }

  TEST(eop_02__distance)
  {
    ASSERT_EQ(0, distance(1u, 1u, incr));
    ASSERT_EQ(1, distance(1u, 2u, incr));
    ASSERT_EQ(4, distance(1u, 5u, incr));
  }

  unsigned int foo(unsigned int value)
  {
    if (value == 30)
    {
      return 18;
    }
    return (value + 1);
  }

  bool foo_pred(unsigned int value)
  {
    return (value < 50);
  }

  TEST(eop_02__collision_point)
  {
    ASSERT_EQ(60, collision_point(60u, foo, foo_pred));

    ASSERT_EQ(51, collision_point(51u, foo, foo_pred));
    ASSERT_EQ(50, collision_point(50u, foo, foo_pred));
    ASSERT_EQ(50, collision_point(49u, foo, foo_pred));

    ASSERT_EQ(50, collision_point(45u, foo, foo_pred));


    ASSERT_EQ(18, collision_point(19u, foo, foo_pred));

    ASSERT_EQ(30, collision_point(18u, foo, foo_pred));

    ASSERT_EQ(29, collision_point(17u, foo, foo_pred));
    ASSERT_EQ(28, collision_point(16u, foo, foo_pred));
  }

  TEST(eop_02__is_terminating)
  {
    ASSERT(is_terminating(60u, foo, foo_pred));

    ASSERT_FALSE(is_terminating(1u, foo, foo_pred));
  }

  TEST(eop_02__collision_point_nonterminating_orbit)
  {
    ASSERT_EQ(18, collision_point_nonterminating_orbit(19u, foo));

    ASSERT_EQ(30, collision_point_nonterminating_orbit(18u, foo));

    ASSERT_EQ(29, collision_point_nonterminating_orbit(17u, foo));
    ASSERT_EQ(28, collision_point_nonterminating_orbit(16u, foo));
  }

  TEST(eop_02__is_circular_nonterminating_orbit)
  {
    ASSERT(is_circular_nonterminating_orbit(19u, foo));

    ASSERT_FALSE(is_circular_nonterminating_orbit(12u, foo));
  }

  TEST(eop_02__is_circular)
  {
    ASSERT_FALSE(is_circular(60u, foo, foo_pred));
    ASSERT_FALSE(is_circular(45u, foo, foo_pred));


    ASSERT(is_circular(19u, foo, foo_pred));

    ASSERT_FALSE(is_circular(16u, foo, foo_pred));
  }

  TEST(eop_02__convergent_point)
  {
    ASSERT_EQ(18, convergent_point(17u, 30u, foo));
  }

  TEST(eop_02_connection_point_nonterminating_orbit)
  {
    ASSERT_EQ(19, connection_point_nonterminating_orbit(19u, foo));
    ASSERT_EQ(18, connection_point_nonterminating_orbit(15u, foo));
  }

  TEST(eop_02_connection_point)
  {
    ASSERT_EQ(60, connection_point(60u, foo, foo_pred));
    ASSERT_EQ(50, connection_point(45u, foo, foo_pred));
    ASSERT_EQ(20, connection_point(20u, foo, foo_pred));
    ASSERT_EQ(18, connection_point(15u, foo, foo_pred));
  }

  TEST(eop_02_orbit_structure_nonterminating_orbit)
  {
    using result_type = triple<DistanceType(foo), DistanceType(foo), unsigned int>;
    ASSERT_EQ(result_type(0u, 12u, 19u), orbit_structure_nonterminating_orbit(19u, foo));
    ASSERT_EQ(result_type(3u, 12u, 18u), orbit_structure_nonterminating_orbit(15u, foo));
  }

  TEST(eop_02_orbit_structure)
  {
    using result_type = triple<DistanceType(foo), DistanceType(foo), unsigned int>;
    ASSERT_EQ(result_type(0u, 0u, 60u), orbit_structure(60u, foo, foo_pred));
    ASSERT_EQ(result_type(5u, 0u, 50u), orbit_structure(45u, foo, foo_pred));
    ASSERT_EQ(result_type(0u, 12u, 19u), orbit_structure(19u, foo, foo_pred));
    ASSERT_EQ(result_type(3u, 12u, 18u), orbit_structure(15u, foo, foo_pred));
  }

} // anonymous namespace
