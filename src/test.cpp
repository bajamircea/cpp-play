#include "test.h"

#include <iostream>
#include <stdexcept>

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
      << ": error: "
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
        std::cout
          << crt_test->file_ << ":" << crt_test->line_
          << ": error: "
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
