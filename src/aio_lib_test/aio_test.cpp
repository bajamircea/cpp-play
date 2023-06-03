#include "../test_lib/test.h"

//#include "../cpp_util_lib/handle_arg.h"

#include <coroutine>
#include <exception>
#include <string>
#include <vector>

namespace
{
  std::vector<std::string> recorder;

  void record(std::string message)
  {
    recorder.push_back(message);
  }

  class task
  {
  public:
    struct promise_type
    {
      task get_return_object() noexcept
      {
        return task(std::coroutine_handle<promise_type>::from_promise(*this));
      }
      std::suspend_always initial_suspend() noexcept { return {}; }
      std::suspend_always final_suspend() noexcept { return {}; }
      void return_void() noexcept {}
      void unhandled_exception() noexcept
      {
        std::terminate();
      }
    };

  private:
    std::coroutine_handle<> coroutine_handle_;

    task(std::coroutine_handle<> coroutine_handle) :
      coroutine_handle_(coroutine_handle)
    {}
  public:
    task(const task &) = delete;
    task & operator=(const task &) = delete;

    task(task && other) noexcept :
      coroutine_handle_{other.coroutine_handle_}
    {
      other.coroutine_handle_ = nullptr;
    }

    task& operator=(task && other) noexcept
    {
      if (this != &other)
      {
        if (coroutine_handle_)
        {
          coroutine_handle_.destroy();
        }
        coroutine_handle_ = other.coroutine_handle_;
        other.coroutine_handle_ = nullptr;
      }
      return *this;
    }

    ~task()
    {
      if (coroutine_handle_)
      {
        coroutine_handle_.destroy();
      }
    }

    bool resume() noexcept
    {
      if (coroutine_handle_.done())
      {
        return false;
      }
      coroutine_handle_.resume();
      return true;
    }
  };

  task foo()
  {
    record("in foo");
    co_return;
  }

  TEST(aio_test)
  {
    recorder.clear();

    task t = foo();
    ASSERT_TRUE(recorder.empty());

    ASSERT_TRUE(t.resume());
    ASSERT_EQ(std::vector<std::string>{"in foo"}, recorder);

    ASSERT_FALSE(t.resume());
    ASSERT_EQ(std::vector<std::string>{"in foo"}, recorder);
  }
} // anonymous namespace