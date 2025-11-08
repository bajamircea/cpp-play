#include "../coro_st_lib/coro_st.h"

#include <iostream>
#include <stdexcept>

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

namespace
{
  test::test_base * head_test = nullptr;
  test::test_base * crt_test = nullptr;

  bool crt_failed;

  struct test_exception
  {
  };

  void log_exception()
  {
    try
    {
      throw;
    }
    catch (const test_exception &)
    {
    }
    catch (const std::runtime_error & e)
    {
      std::cout << "  caught std::runtime_error: " << e.what() << "\n";
    }
    catch (const std::exception & e)
    {
      std::cout << "  caught std::exception: " << e.what() << "\n";
    }
    catch (...)
    {
      std::cout << "  caught unknown exception\n";
    }
  }

} // anonymous namespace

namespace test
{
  test_base::test_base(test_fn fn, const char * name, const char * file, int line) :
    fn_{ fn }, name_{ name }, file_{ file }, line_{ line }, next_{ head_test }
  {
    head_test = this;
  }

  void fail_current(const char * file, int line, const char * message)
  {
    crt_failed = true;
    std::cout
      << file << ":" << line
      << ":1: error: "
      << crt_test->name_ << " " << message << "\n";
    throw test_exception();
  }

  int run()
  {
    bool any_failed = false;

    for (crt_test = head_test;
        crt_test != nullptr;
        crt_test = crt_test->next_)
    {
      crt_failed = false;

      std::cout << "test "
        << crt_test->name_ << "\n";

      try
      {
        crt_test->fn_();
      }
      catch(...)
      {
        crt_failed = true;
        log_exception();
      }

      if (crt_failed)
      {
        any_failed = true;
        std::cout
          << crt_test->file_ << ":" << crt_test->line_
          << ":1: error: "
          << crt_test->name_ << " failed\n";
      }
    }

    if (any_failed)
    {
      std::cout << "[FAILED]\n";
    }
    else
    {
      std::cout << "[OK]\n";
    }

    return any_failed ? 1 : 0;
  }
} // namespace test

namespace
{
  template<coro_st::is_co_task CoTask>
  void run2(CoTask co_task)
  {
    coro_st::stop_source main_stop_source;

    coro_st::context ctx{ main_stop_source.get_token() };

    auto co_awaiter = co_task.get_work().get_awaiter(ctx);

    co_awaiter.start_as_chain_root();
  }

  // For some reason that triggers what I believe to be a false positive
  // on g++ that made me use -Wno-dangling-pointer on g++ -O3 build
  TEST(stop_when_exception2)
  {
    auto async_lambda = []() -> coro_st::co {
      co_return;
    };

    run2(coro_st::async_stop_when(
      coro_st::async_suspend_forever(),
      async_lambda()
    ));
  }

} // anonymous namespace

int main()
{
  return test::run();
}
