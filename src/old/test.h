#pragma once

namespace test
{
  using test_fn = void(*)();

  struct test_base
  {
    test_base(test_fn fn, const char * name, const char * file, int line);

    test_fn fn_;
    const char * name_;
    const char * file_;
    const int line_;
    test_base * next_;
  };

  void fail_current(const char * file, int line, const char * message);

  int run();
} // namespace test

#define TEST(func) void func(); test::test_base func ## _instance{func, #func, __FILE__, __LINE__}; void func()

#define FAIL() test::fail_current(__FILE__, __LINE__, "FAILED was called")

#define ASSERT_TRUE(cond) if (cond) {} else { test::fail_current(__FILE__, __LINE__, "ASSERT failed '" #cond "' was true"); }

#define ASSERT_FALSE(cond) if (cond) { test::fail_current(__FILE__, __LINE__, "ASSERT_FALSE failed '" #cond "' was false"); }

#define ASSERT_EQ(expected, actual) if (expected == actual) {} else { test::fail_current(__FILE__, __LINE__, "ASSERT_EQ failed for expected: '" #expected "' and actual '" #actual "'"); }

#define ASSERT_NE(expected, actual) if (expected == actual) { test::fail_current(__FILE__, __LINE__, "ASSERT_NE failed for expected: '" #expected "' and actual '" #actual "'"); }

#define ASSERT_THROW(expr, ex) try{ {expr;} test::fail_current(__FILE__, __LINE__, "ASSERT_THROW failed for expression '" #expr "'"); } catch (const ex &) {}

