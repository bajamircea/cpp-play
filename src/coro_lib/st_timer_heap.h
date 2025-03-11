#pragma once

#include "../cpp_util_lib/intrusive_heap.h"

#include <chrono>
#include <coroutine>

namespace coro::st
{
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
}