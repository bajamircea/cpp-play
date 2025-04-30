#include "../test_lib/test.h"

#include "../coro_st_lib/call_capture.h"

#include "../coro_st_lib/co.h"

#include <string>

namespace
{
  int foo_void()
  {
    return 42;
  }

  TEST(call_capture_simple)
  {
    coro_st::call_capture x(foo_void);

    ASSERT_EQ(42, x());
  }

  std::string foo_takes_value(int i)
  {
    return std::to_string(i);
  }

  TEST(call_capture_value)
  {
    coro_st::call_capture x(foo_takes_value, 42);

    ASSERT_EQ("42", x());
  }

  std::string foo_takes_ref(int& i)
  {
    return std::to_string(i);
  }

  TEST(call_capture_ref)
  {
    int i = 42;
    coro_st::call_capture x(foo_takes_ref, i);
    i = 43;
    // reference is to the copy inside call_capture
    ASSERT_EQ("42", x());
  }

  TEST(call_capture_ref2)
  {
    int i = 42;
    coro_st::call_capture x(foo_takes_ref, std::ref(i));
    i = 43;

    ASSERT_EQ("43", x());
  }

  std::string foo_takes_const_ref(const int& i)
  {
    return std::to_string(i);
  }

  TEST(call_capture_const_ref)
  {
    coro_st::call_capture x(foo_takes_const_ref, 42);

    ASSERT_EQ("42", x());
  }

  // std::string foo_takes_temporary(int&& i)
  // {
  //   return std::to_string(i);
  // }

  // TEST(call_capture_const_temp)
  // {
  //   coro_st::call_capture x(foo_takes_temporary, 42);

  //   ASSERT_EQ("42", x());
  // }

  struct Obj
  {
    int i_{};
    void foo_member(const int& i)
    {
      i_ = i;
    }
  };

  TEST(call_capture_member)
  {
    Obj o;

    coro_st::call_capture x(&Obj::foo_member, &o, 42);
    x();

    ASSERT_EQ(42, o.i_);
  }

  // TODO: lambda
} // anonymous namespace