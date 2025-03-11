#pragma once

#include "co.h"
#include "deferred_co.h"
#include "promise_base.h"
#include "st_stop.h"

#include "../cpp_util_lib/intrusive_heap.h"
#include "../cpp_util_lib/intrusive_queue.h"

#include <chrono>
#include <thread>

namespace coro::st
{
  struct ready_node
  {
    ready_node() noexcept = default;

    ready_node(const ready_node&) = delete;
    ready_node& operator=(const ready_node&) = delete;

    ready_node* next{};
    std::coroutine_handle<> coroutine;
  };

  using ready_queue = cpp_util::intrusive_queue<ready_node, &ready_node::next>;

  struct timer_node
  {
    timer_node() noexcept = default;

    timer_node(const timer_node&) = delete;
    timer_node& operator=(const timer_node&) = delete;

    timer_node* parent{};
    timer_node* left{};
    timer_node* right{};
    std::coroutine_handle<> coroutine;
    std::chrono::steady_clock::time_point deadline;
  };

  struct compare_timer_node_by_deadline
  {
    bool operator()(const timer_node& left, const timer_node& right) noexcept
    {
      return left.deadline < right.deadline;
    }
  };

  using timer_heap = cpp_util::intrusive_heap<timer_node, &timer_node::parent, &timer_node::left, &timer_node::right, compare_timer_node_by_deadline>;

  template<typename T>
  class [[nodiscard]] trampoline_co
  {
  public:
    using co_return_type = T;

    class promise_type : public promise_base<T>
    {
      friend trampoline_co;

    public:
      promise_type() noexcept = default;

      promise_type(const promise_type&) = delete;
      promise_type& operator=(const promise_type&) = delete;

      trampoline_co get_return_object() noexcept
      {
        return trampoline_co{std::coroutine_handle<promise_type>::from_promise(*this)};
      }

      std::suspend_never initial_suspend() noexcept
      {
        return {};
      }

      std::suspend_always final_suspend() noexcept
      {
        return {};
      }
    };

  private:
    unique_coroutine_handle<promise_type> unique_child_coro_;

    trampoline_co(std::coroutine_handle<promise_type> child_coro) noexcept : unique_child_coro_{ child_coro }
    {
    }

  public:
    trampoline_co(const trampoline_co&) = delete;
    trampoline_co& operator=(const trampoline_co&) = delete;

    bool done() const noexcept
    {
      return unique_child_coro_.get().done();
    }

    T get_result()
    {
      return unique_child_coro_.get().promise().get_result();
    }
  };

  class context
  {
    coro::st::stop_token token_;
    ready_queue& ready_queue_;
    timer_heap& timers_heap_;

  public:
    context(coro::st::stop_token token, ready_queue& ready_queue, timer_heap& timers_heap) noexcept :
      token_{ token }, ready_queue_{ ready_queue }, timers_heap_{ timers_heap }
    {
    }

    context(coro::st::stop_token token, context& other) noexcept :
    token_{ token }, ready_queue_{ other.ready_queue_ }, timers_heap_{ other.timers_heap_ }
    {
    }

    context(const context &) = delete;
    context & operator=(const context &) = delete;

    void push_ready_node(ready_node& node, std::coroutine_handle<> handle) noexcept
    {
      node.coroutine = handle;
      ready_queue_.push(&node);
    }

    void insert_timer_node(timer_node& node, std::coroutine_handle<> handle) noexcept
    {
      node.coroutine = handle;
      timers_heap_.insert(&node);
    }
  };

  class scheduler
  {
    ready_queue ready_queue_;
    timer_heap timers_heap_;

  public:
    scheduler() noexcept = default;

    scheduler(const scheduler &) = delete;
    scheduler & operator=(const scheduler &) = delete;

    template<typename DeferredCoFn>
    auto trampoline(context& ctx, DeferredCoFn&& co_fn) -> trampoline_co<typename DeferredCoFn::co_return_type>
    {
      co_return co_await co_fn(ctx);
    }

    template<typename DeferredCoFn>
    auto run(DeferredCoFn&& co_fn) -> DeferredCoFn::co_return_type
    {
      stop_source root_stop_source;
      context root_ctx(root_stop_source.get_token(), ready_queue_, timers_heap_);

      auto root_co = trampoline(root_ctx, std::forward<DeferredCoFn>(co_fn));

      while (!root_co.done())
      {
        do_work();
      }

      return root_co.get_result();
    }

  private:
    void do_work() {
      cpp_util::intrusive_queue local_ready = std::move(ready_queue_);
      while (!local_ready.empty())
      {
        auto* ready_node = local_ready.pop();
        ready_node->coroutine.resume();
      }
      if (timers_heap_.min_node() != nullptr)
      {
        auto now = std::chrono::steady_clock::now();
        do
        {
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
        } while(timers_heap_.min_node() != nullptr);
      }
    }
  };
}