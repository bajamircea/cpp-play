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

#define FAIL_TEST(reason) test::fail_current(__FILE__, __LINE__, "FAILED_TEST(" #reason ")")

#define ASSERT_TRUE(cond) if (cond) {} else { test::fail_current(__FILE__, __LINE__, "ASSERT failed '" #cond "' was false"); }

#define ASSERT_FALSE(cond) if (cond) { test::fail_current(__FILE__, __LINE__, "ASSERT_FALSE failed '" #cond "' was true"); }

#define ASSERT_EQ(expected, actual) if (expected == actual) {} else { test::fail_current(__FILE__, __LINE__, "ASSERT_EQ failed for expected: '" #expected "' and actual '" #actual "'"); }

#define ASSERT_NE(expected, actual) if (expected == actual) { test::fail_current(__FILE__, __LINE__, "ASSERT_NE failed for expected: '" #expected "' and actual '" #actual "'"); }

#define ASSERT_THROW(expr, ex) try{ {expr;} test::fail_current(__FILE__, __LINE__, "ASSERT_THROW failed for expression '" #expr "'"); } catch (const ex &) {}

#define ASSERT_NO_THROW(expr) { bool assert_no_throw = true; try{ {expr;} assert_no_throw = false; } catch(...) {} if (assert_no_throw) { test::fail_current(__FILE__, __LINE__, "ASSERT_NO_THROW failed for expression '" #expr "'"); }

#define ASSERT_THROW_WHAT(expr, ex, what_str) try{ {expr;} test::fail_current(__FILE__, __LINE__, "ASSERT_THROW_WHAT failed for expression '" #expr "'"); } catch (const ex & e) { ASSERT_EQ(what_str, std::string(e.what())); }