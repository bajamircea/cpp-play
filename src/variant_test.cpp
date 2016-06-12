#include "variant.h"

#include "test.h"

using namespace variant;

TEST(variant__use_with_int_bool)
{
  using pib = ptr<int, bool>;

  struct detect_int
  {
    explicit detect_int(int & v) :
      v_{ v }
    {
    }

    void operator()(int & v)
    {
      v_ = v;
    }

    void operator()(bool &)
    {
      FAIL();
    }

    int & v_;
  };

  struct detect_bool
  {
    explicit detect_bool(bool & v) :
      v_{ v }
    {
    }

    void operator()(int &)
    {
      FAIL();
    }

    void operator()(bool & v)
    {
      v_ = v;
    }

    bool & v_;
  };

  pib a(42);
  int actual_int = 0;
  a.apply(detect_int(actual_int));
  ASSERT_EQ(42, actual_int);

  pib b(true);
  bool actual_bool = false;
  a.apply(detect_bool(actual_bool));
  ASSERT_EQ(true, actual_bool);

}
