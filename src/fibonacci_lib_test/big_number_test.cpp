#include "../test_lib/test.h"

#include "../fibonacci_lib/big_number.h"

namespace
{
  using namespace fibonacci::big_number;

  TEST(big_number_digit)
  {
    ASSERT_EQ(3, make_digit('3'));
    ASSERT_EQ('3', from_digit(3));
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
      unsigned_decimal a = make_unsigned_decimal("9");
      unsigned_decimal b = make_unsigned_decimal("99");
      unsigned_decimal c = make_unsigned_decimal("108");
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