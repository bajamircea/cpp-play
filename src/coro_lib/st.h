#pragma once

#include "../coro_lib/task.h"
#include "../cpp_util_lib/intrusive_heap.h"
#include "../cpp_util_lib/intrusive_list.h"
#include "../cpp_util_lib/intrusive_queue.h"

#include <chrono>
#include <thread>

namespace coro::st
{
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
    std::chrono::steady_clock::time_point deadline;
  };

  struct compare_timer_node_by_deadline
  {
    bool operator()(const timer_node & left, const timer_node & right)
    {
      return left.deadline < right.deadline;
    }
  };

  using timer_heap = cpp_util::intrusive_heap<timer_node, &timer_node::parent, &timer_node::left, &timer_node::right, compare_timer_node_by_deadline>;

  class context
  {
    class spawn_task
    {
    public:
      class promise_type
      {
        context & context_;
        scope_node scope_node_;

      public:
        template<typename Arg>
        promise_type(context & context, Arg&):
          context_{ context }
        {
        }

        spawn_task get_return_object() noexcept
        {
          return spawn_task{};
        }

        struct initial_await
        {
          ready_node ready_node_;

          [[nodiscard]] constexpr bool await_ready() const noexcept
          {
            return false;
          }

          void await_suspend(std::coroutine_handle<promise_type> handle) noexcept
          {
            ready_node_.coroutine = handle;
            auto & promise = handle.promise();
            promise.scope_node_.coroutine = handle;
            promise.context_.ready_queue_.push(&ready_node_);
            promise.context_.scope_list_.push_back(&promise.scope_node_);
          }

          constexpr void await_resume() const noexcept {}
        };
        
        initial_await initial_suspend() noexcept
        {
          return {};
        }

        struct final_await
        {
          [[nodiscard]] constexpr bool await_ready() const noexcept
          {
            return false;
          }

          void await_suspend(std::coroutine_handle<promise_type> handle) noexcept
          {
            auto & promise = handle.promise();
            promise.context_.scope_list_.remove(&promise.scope_node_);
            handle.destroy();
          }

          constexpr void await_resume() const noexcept {}
        };

        final_await final_suspend() noexcept
        {
          return {};
        }

        constexpr void return_void() const noexcept
        {
        }

        [[noreturn]] /*constexpr*/ void unhandled_exception() const noexcept
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
      context & context_;
      timer_node node_;

      sleep_await(context & context, std::chrono::steady_clock::time_point deadline) :
        context_{ context }
      {
        node_.deadline = deadline;
      }

      [[nodiscard]] constexpr bool await_ready() const noexcept
      {
        return false;
      }

      void await_suspend(std::coroutine_handle<> handle) noexcept
      {
        node_.coroutine = handle;
        context_.timers_heap_.insert(&node_);
      }

      constexpr void await_resume() const noexcept
      {
      }
    };

    ready_queue ready_queue_;
    timer_heap timers_heap_;
    scope_list scope_list_;

  public:
    context()
    {
    }
    context(const context &) = delete;
    context & operator=(const context &) = delete;
    ~context()
    {
      scope_node * crt = scope_list_.back();
      while (crt != nullptr)
      {
        scope_node * tmp = crt;
        crt = crt->prev;
        tmp->coroutine.destroy();
      }
    }

    spawn_task spawn_coro(task<void> t)
    {
      co_await std::move(t);
    }

  public:
    void spawn(task<void> && t)
    {
      spawn_coro(std::move(t));
    }

    sleep_await sleep(std::chrono::steady_clock::duration sleep_duration)
    {
      return sleep_await(*this, std::chrono::steady_clock::now() + sleep_duration);
    }

    void run()
    {
      while (true)
      {
        if (ready_queue_.empty() && timers_heap_.empty())
        {
          return;
        }
        cpp_util::intrusive_queue local_ready = std::move(ready_queue_);
        while(!local_ready.empty())
        {
          auto* ready_node = local_ready.pop();
          ready_node->coroutine.resume();
        }
        while (timers_heap_.min_node() != nullptr)
        {
          auto now = std::chrono::steady_clock::now();
          auto* timer_node = timers_heap_.min_node();
          if (timer_node->deadline > now)
          {
            if (ready_queue_.empty())
            {
              std::this_thread::sleep_for(timer_node->deadline - now);
            }
            break;
          }
          timers_heap_.pop_min();
          timer_node->coroutine.resume();
        }
      }
    }
  };
}
