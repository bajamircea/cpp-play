#include "../test_lib/test.h"

#include "../fibonacci_lib/big_number.h"

namespace
{
  using namespace fibonacci::big_number;

  TEST(big_number_digit)
  {
    ASSERT_EQ(3, make_digit('3'));
  }

  TEST(big_number_unsigned_binary_copy_assign)
  {
    {
      unsigned_binary a;
    }
    {
      unsigned_binary a;
      unsigned_binary b = a;
      ASSERT_EQ(a, b);
    }
    {
      unsigned_binary a;
      unsigned_binary b = std::move(a);
    }
    {
      unsigned_binary a;
      unsigned_binary b;
      b = a;
      ASSERT_EQ(a, b);
    }
    {
      unsigned_binary a;
      unsigned_binary b;
      b = std::move(a);
    }
  }

  TEST(big_number_unsigned_binary_order)
  {
    {
      unsigned_binary a;
      unsigned_binary b;
      ASSERT_FALSE(a < b);
      ASSERT(a <= b);
      ASSERT_FALSE(a > b);
      ASSERT(a >= b);
    }
    {
      unsigned_binary a(12345);
      unsigned_binary b(12345);
      ASSERT_FALSE(a < b);
      ASSERT(a <= b);
      ASSERT_FALSE(a > b);
      ASSERT(a >= b);
    }
    {
      unsigned_binary a;
      unsigned_binary b(12345);
      ASSERT(a < b);
      ASSERT(a <= b);
      ASSERT_FALSE(a > b);
      ASSERT_FALSE(a >= b);
    }
    {
      unsigned_binary a(12345);
      unsigned_binary b(12351);
      ASSERT(a < b);
      ASSERT(a <= b);
      ASSERT_FALSE(a > b);
      ASSERT_FALSE(a >= b);
    }
  }

  TEST(big_number_unsigned_binary_add_substract)
  {
    {
      unsigned_binary a;
      unsigned_binary b = make_unsigned_binary("12345");
      ASSERT_EQ(b, a + b);
      ASSERT_EQ(b, b + a);
      ASSERT_EQ(b, b - a);
      ASSERT_EQ(a, b - b);
    }
    {
      unsigned_binary a = make_unsigned_binary("1");
      unsigned_binary b = make_unsigned_binary("12345");
      unsigned_binary c = make_unsigned_binary("12346");
      ASSERT_EQ(c, a + b);
      ASSERT_EQ(c, b + a);
      ASSERT_EQ(a, c - b);
      ASSERT_EQ(b, c - a);
    }
    {
      unsigned_binary a = make_unsigned_binary("23456789012345678901");
      unsigned_binary b = make_unsigned_binary("23456789012345678901");
      unsigned_binary c = make_unsigned_binary("46913578024691357802");
      ASSERT_EQ(c, a + b);
      ASSERT_EQ(c, b + a);
      ASSERT_EQ(a, c - b);
    }
    {
      unsigned_binary a(1);
      unsigned_binary b = make_unsigned_binary("4294967295");
      unsigned_binary c = make_unsigned_binary("4294967296");
      ASSERT_EQ(c, a + b);
      ASSERT_EQ(c, b + a);
      ASSERT_EQ(a, c - b);
      ASSERT_EQ(b, c - a);
    }
    {
      unsigned_binary a(1);
      unsigned_binary b = make_unsigned_binary("4294967296");
      unsigned_binary c = make_unsigned_binary("4294967297");
      ASSERT_EQ(c, a + b);
      ASSERT_EQ(c, b + a);
      ASSERT_EQ(a, c - b);
      ASSERT_EQ(b, c - a);
    }
    {
      unsigned_binary a(1);
      unsigned_binary b;
      b.units_ = {max_unit, max_unit, max_unit};
      unsigned_binary c;
      c.units_ = {0, 0, 0, 1};
      ASSERT_EQ(c, a + b);
      ASSERT_EQ(c, b + a);
      ASSERT_EQ(a, c - b);
      ASSERT_EQ(b, c - a);
    }
    {
      unsigned_binary a(1);
      unsigned_binary b;
      b.units_ = {max_unit, max_unit, max_unit, 1};
      unsigned_binary c;
      c.units_ = {0, 0, 0, 2};
      ASSERT_EQ(c, a + b);
      ASSERT_EQ(c, b + a);
      ASSERT_EQ(a, c - b);
      ASSERT_EQ(b, c - a);
    }
  }

  TEST(big_number_long_multiplication)
  {
    {
      unsigned_binary a;
      unsigned_binary b = make_unsigned_binary("12345");
      ASSERT_EQ(a.units_, long_multiplication(a.units_, b.units_));
      ASSERT_EQ(a.units_, long_multiplication(b.units_, a.units_));
    }
    {
      unsigned_binary a = make_unsigned_binary("1");
      unsigned_binary b = make_unsigned_binary("12345");
      ASSERT_EQ(b.units_, long_multiplication(a.units_, b.units_));
      ASSERT_EQ(b.units_, long_multiplication(b.units_, a.units_));
    }
    {
      unsigned_binary a = make_unsigned_binary("23456789012345678901");
      unsigned_binary b = make_unsigned_binary("23456789012345678901");
      unsigned_binary c = make_unsigned_binary("550220950769700970237433565526596567801");
      ASSERT_EQ(c.units_, long_multiplication(a.units_, b.units_));
      ASSERT_EQ(c.units_, long_multiplication(b.units_, a.units_));
    }
    {
      unsigned_binary a(1);
      unsigned_binary b = make_unsigned_binary("4294967295");
      ASSERT_EQ(b.units_, long_multiplication(a.units_, b.units_));
      ASSERT_EQ(b.units_, long_multiplication(b.units_, a.units_));
    }
    {
      unsigned_binary a = make_unsigned_binary("4294967295");
      unsigned_binary b(2);
      unsigned_binary c = a + a;
      ASSERT_EQ(c.units_, long_multiplication(a.units_, b.units_));
      ASSERT_EQ(c.units_, long_multiplication(b.units_, a.units_));
    }
  }

  TEST(big_number_unsigned_binary_divide_by)
  {
    {
      unsigned_binary a;
      unsigned_binary zero;
      ASSERT_EQ(0, a.divide_by(1000000000));
      ASSERT_EQ(zero, a);
    }
    {
      unsigned_binary a = make_unsigned_binary("12345678901");
      unsigned_binary b = make_unsigned_binary("12");
      ASSERT_EQ(345678901, a.divide_by(1000000000));
      ASSERT_EQ(b, a);
    }
    {
      unsigned_binary a = make_unsigned_binary("12000000000");
      unsigned_binary b = make_unsigned_binary("12");
      ASSERT_EQ(0, a.divide_by(1000000000));
      ASSERT_EQ(b, a);
    }
  }

  TEST(big_number_unsigned_decimal_copy_assign)
  {
    {
      unsigned_decimal a;
    }
    {
      unsigned_decimal a;
      unsigned_decimal b = a;
      ASSERT_EQ(a, b);
    }
    {
      unsigned_decimal a;
      unsigned_decimal b = std::move(a);
    }
    {
      unsigned_decimal a;
      unsigned_decimal b;
      b = a;
      ASSERT_EQ(a, b);
    }
    {
      unsigned_decimal a;
      unsigned_decimal b;
      b = std::move(a);
    }
  }

  TEST(big_number_unsigned_decimal_equality)
  {
    {
      unsigned_decimal a;
      unsigned_decimal b;
      ASSERT_EQ(a, b);
    }
    {
      unsigned_decimal a = make_unsigned_decimal("12345");
      unsigned_decimal b = make_unsigned_decimal("12345");
      ASSERT_EQ(a, b);
    }
    {
      unsigned_decimal a;
      unsigned_decimal b = make_unsigned_decimal("12345");
      ASSERT_NE(a, b);
    }
  }

  TEST(big_number_unsigned_decimal_order)
  {
    {
      unsigned_decimal a;
      unsigned_decimal b;
      ASSERT_FALSE(a < b);
      ASSERT(a <= b);
      ASSERT_FALSE(a > b);
      ASSERT(a >= b);
    }
    {
      unsigned_decimal a = make_unsigned_decimal("12345");
      unsigned_decimal b = make_unsigned_decimal("12345");
      ASSERT_FALSE(a < b);
      ASSERT(a <= b);
      ASSERT_FALSE(a > b);
      ASSERT(a >= b);
    }
    {
      unsigned_decimal a;
      unsigned_decimal b = make_unsigned_decimal("12345");
      ASSERT(a < b);
      ASSERT(a <= b);
      ASSERT_FALSE(a > b);
      ASSERT_FALSE(a >= b);
    }
    {
      unsigned_decimal a = make_unsigned_decimal("12345");
      unsigned_decimal b = make_unsigned_decimal("12351");
      ASSERT(a < b);
      ASSERT(a <= b);
      ASSERT_FALSE(a > b);
      ASSERT_FALSE(a >= b);
    }
  }

  TEST(big_number_unsigned_decimal_add)
  {
    {
      unsigned_decimal a;
      unsigned_decimal b = make_unsigned_decimal("12345");
      ASSERT_EQ(b, a + b);
      ASSERT_EQ(b, b + a);
    }
    {
      unsigned_decimal a = make_unsigned_decimal("1");
      unsigned_decimal b = make_unsigned_decimal("12345");
      unsigned_decimal c = make_unsigned_decimal("12346");
      ASSERT_EQ(c, a + b);
      ASSERT_EQ(c, b + a);
    }
    {
      unsigned_decimal a = make_unsigned_decimal("999999999");
      unsigned_decimal b = make_unsigned_decimal("999999999999999999");
      unsigned_decimal c = make_unsigned_decimal("1000000000999999998");
      ASSERT_EQ(c, a + b);
      ASSERT_EQ(c, b + a);
    }
    {
      unsigned_decimal a = make_unsigned_decimal("9");
      unsigned_decimal b = make_unsigned_decimal("109");
      unsigned_decimal c = make_unsigned_decimal("118");
      ASSERT_EQ(c, a + b);
      ASSERT_EQ(c, b + a);
    }
  }

  TEST(big_number_unsigned_decimal_halve)
  {
    {
      unsigned_decimal a;
      unsigned_decimal zero;
      ASSERT_EQ(0, a.halve());
      ASSERT_EQ(zero, a);
    }
    {
      unsigned_decimal a = make_unsigned_decimal("12345");
      unsigned_decimal b = make_unsigned_decimal("6172");
      ASSERT_EQ(1, a.halve());
      ASSERT_EQ(b, a);
    }
    {
      unsigned_decimal a = make_unsigned_decimal("90");
      unsigned_decimal b = make_unsigned_decimal("45");
      ASSERT_EQ(0, a.halve());
      ASSERT_EQ(b, a);
    }
  }

  TEST(big_number_make_unsigned_binary)
  {
    {
      unsigned_binary a = make_unsigned_binary("");
      ASSERT(a.units_.empty());
    }
    {
      unsigned_binary a = make_unsigned_binary("12345");
      unsigned_binary b(12345U);
      ASSERT_EQ(a, b);
    }
    {
      unsigned_binary a = make_unsigned_binary("4294967295");
      unsigned_binary b(4294967295U);
      ASSERT_EQ(a, b);
    }
    {
      unsigned_binary a = make_unsigned_binary("18446744073709551615");
      unsigned_decimal b = make_unsigned_decimal("18446744073709551615");
      ASSERT_EQ(b, make_unsigned_decimal(a));
    }
    {
      unsigned_binary a = make_unsigned_binary("23456789012345678901");
      unsigned_decimal b = make_unsigned_decimal("23456789012345678901");
      ASSERT_EQ(b, make_unsigned_decimal(a));
    }
  }

  TEST(big_number_make_unsigned_binary_from_to_string)
  {
    {
      unsigned_binary x = make_unsigned_binary("12345");
      ASSERT_EQ("12345", to_string(x));
    }
    {
      unsigned_binary x = make_unsigned_binary("90");
      ASSERT_EQ("90", to_string(x));
    }
    {
      unsigned_binary x = make_unsigned_binary("");
      ASSERT_EQ("0", to_string(x));
    }
  }

  TEST(big_number_make_unsigned_decimal_from_unsigned_binary)
  {
    {
      unsigned_binary a;
      unsigned_decimal b = make_unsigned_decimal(a);
      ASSERT_EQ("0", to_string(b));
    }
    {
      unsigned_binary a(12345);
      unsigned_decimal b = make_unsigned_decimal(a);
      ASSERT_EQ("12345", to_string(b));
    }
  }

  TEST(big_number_make_unsigned_decimal_from_to_string)
  {
    {
      unsigned_decimal x = make_unsigned_decimal("12345");
      ASSERT_EQ("12345", to_string(x));
    }
    {
      unsigned_decimal x = make_unsigned_decimal("90");
      ASSERT_EQ("90", to_string(x));
    }
    {
      unsigned_decimal x = make_unsigned_decimal("");
      ASSERT_EQ("0", to_string(x));
    }
  }
} // anonymous namespace