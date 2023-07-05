#include "../test_lib/test.h"

#include "../cpp_util_lib/intrusive_heap.h"
#include "../cpp_util_lib/intrusive_list.h"
#include "../cpp_util_lib/intrusive_queue.h"

#include <chrono>
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
    class promise_type
    {
      std::coroutine_handle<> continuation_ = nullptr;

    public:
      task get_return_object() noexcept
      {
        return task{ std::coroutine_handle<promise_type>::from_promise(*this) };
      }
      
      std::suspend_always initial_suspend() noexcept
      {
        return {};
      }

      struct final_await
      {
        constexpr bool await_ready() const noexcept { return false; }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> handle) noexcept
        {
          return handle.promise().get_continuation();
        }

        constexpr void await_resume() const noexcept {}
      };

      final_await final_suspend() noexcept
      {
        return {};
      }

      void return_void() noexcept
      {
      }

      void unhandled_exception() noexcept
      {
        std::terminate();
      }

      void set_continuation(std::coroutine_handle<> continuation) noexcept 
      {
        continuation_ = continuation;
      }

      std::coroutine_handle<> get_continuation() noexcept 
      {
        return continuation_;
      }
    };

  private:
    std::coroutine_handle<promise_type> handle_;

    explicit task(std::coroutine_handle<promise_type> handle) noexcept :
      handle_{ handle }
    {
    }

  public:
    task(const task &) = delete;
    task & operator=(const task &) = delete;

    task(task && other) noexcept : handle_{ other.handle_ }
    {
      other.handle_ = nullptr;
    }

    task & operator=(task && other) noexcept
    {
      if (this != & other)
      {
        if (handle_)
        {
          handle_.destroy();
        }
        handle_ = other.handle_;
        other.handle_ = nullptr;
      }
      return *this;
    }

    ~task()
    {
      if (handle_)
      {
        handle_.destroy();
      }
    }

    constexpr bool await_ready() const noexcept { return false; }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation) noexcept
    {
      handle_.promise().set_continuation(continuation);
      return handle_;
    }

    constexpr void await_resume() const noexcept {}
  };

  struct ready_node
  {
    ready_node() noexcept = default;
    ready_node(const ready_node &) = delete;
    ready_node & operator=(const ready_node &) = delete;

    ready_node * next;
    std::coroutine_handle<> coroutine;
  };

  using ready_queue = cpp_util::intrusive_queue<ready_node, &ready_node::next>;

  struct scope_node
  {
    scope_node() noexcept = default;
    scope_node(const scope_node &) = delete;
    scope_node & operator=(const scope_node &) = delete;

    scope_node * next;
    scope_node * prev;
    std::coroutine_handle<> coroutine;
  };

  using scope_list = cpp_util::intrusive_list<scope_node, &scope_node::next, &scope_node::prev>;

  struct timer_node
  {
    timer_node() noexcept = default;
    timer_node(const timer_node &) = delete;
    timer_node & operator=(const timer_node &) = delete;

    timer_node * parent;
    timer_node * left;
    timer_node * right;
    std::coroutine_handle<> coroutine;
    std::chrono::steady_clock::time_point when;
  };

  struct compare_timer_node
  {
    bool operator()(const timer_node & left, const timer_node & right)
    {
      return left.when < right.when;
    }
  };

  using timer_heap = cpp_util::intrusive_heap<timer_node, &timer_node::parent, &timer_node::left, &timer_node::right, compare_timer_node>;

  class spawn_task
  {
  public:
    class promise_type
    {
      ready_queue & ready_;
      scope_list & scope_;
      scope_node scope_node_;
    public:
      template<typename ...Args>
      promise_type(ready_queue & ready, scope_list & scope, Args&& ...):
        ready_{ ready },
        scope_{ scope }
      {
      }

      spawn_task get_return_object() noexcept
      {
        return spawn_task{};
      }

      struct initial_await
      {
        ready_node ready_node_;

        constexpr bool await_ready() const noexcept { return false; }

        void await_suspend(std::coroutine_handle<promise_type> handle) noexcept
        {
          ready_node_.coroutine = handle;
          auto & promise = handle.promise();
          promise.scope_node_.coroutine = handle;
          handle.promise().ready_.push(&ready_node_);
          promise.scope_.push_back(&promise.scope_node_);
        }

        constexpr void await_resume() const noexcept {}
      };
      
      initial_await initial_suspend() noexcept
      {
        return {};
      }

      struct final_await
      {
        constexpr bool await_ready() const noexcept { return false; }

        bool await_suspend(std::coroutine_handle<promise_type> handle) noexcept
        {
          auto & promise = handle.promise();
          promise.scope_.remove(&promise.scope_node_);
          return false;
        }

        constexpr void await_resume() const noexcept {}
      };

      final_await final_suspend() noexcept
      {
        return {};
      }

      void return_void() noexcept
      {
      }

      void unhandled_exception() noexcept
      {
        std::terminate();
      }
    };

  private:
    explicit spawn_task() noexcept
    {
    }
  };

  struct sleep_await
  {
    timer_heap & timers_;
    timer_node node_;

    sleep_await(timer_heap & timers, std::chrono::steady_clock::time_point when) :
      timers_{ timers }
    {
      node_.when = when;
    }

    constexpr bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) noexcept
    {
      node_.coroutine = handle;
      timers_.insert(&node_);
    }

    constexpr void await_resume() const noexcept {}
  };

  class io_service
  {
    ready_queue ready_;
    scope_list scope_;
    timer_heap timers_;

  public:
    io_service()
    {
    }
    io_service(const io_service &) = delete;
    io_service & operator=(const io_service &) = delete;
    ~io_service()
    {
      scope_node * crt = scope_.back();
      while (crt != nullptr)
      {
        scope_node * tmp = crt;
        crt = crt->prev;
        tmp->coroutine.destroy();
      }
    }

    static spawn_task spawn_coro(ready_queue &, scope_list &, task t)
    {
      co_await t;
    }

  public:
    void spawn(task && t)
    {
      spawn_coro(ready_, scope_, std::move(t));
      return;
    }

    void run()
    {
      while (true)
      {
        while(!ready_.empty())
        {
          auto ready_node = ready_.pop();
          ready_node->coroutine.resume();
        }
        if (timers_.empty())
        {
          return;
        }
        timer_node * sleeper = timers_.min_node();
        timers_.pop_min();
        sleeper->coroutine.resume();
      }
    }

    sleep_await sleep(std::chrono::steady_clock::duration sleep_duration)
    {
      return sleep_await(timers_, std::chrono::steady_clock::now() + sleep_duration);
    }
  };

  task bar(io_service & io, int i)
  {
    record("start " + std::to_string(i));
    co_await io.sleep(std::chrono::seconds(i));
    record("end " + std::to_string(i));
  }

  task foo(io_service & io)
  {
    record("in foo");

    for (int i = 0 ; i < 3 ; ++i)
    {
      io.spawn(bar(io, 3 - i));
    }

    co_return;
  }

  TEST(aio_test)
  {
    recorder.clear();

    io_service io;

    io.spawn(foo(io));

    ASSERT_TRUE(recorder.empty());

    io.run();

    std::vector<std::string> expected{"in foo", "start 3", "start 2", "start 1", "end 1", "end 2", "end 3"};
    ASSERT_EQ(expected, recorder);
  }
} // anonymous namespace