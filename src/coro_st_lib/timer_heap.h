#pragma once

#include "../cpp_util_lib/intrusive_heap.h"

#include <chrono>

namespace coro_st
{
  struct timer_node
  {
    timer_node() noexcept = default;
    timer_node(std::chrono::steady_clock::time_point deadline_arg) noexcept :
      deadline{ deadline_arg }
    {
    }

    timer_node(const timer_node&) = delete;
    timer_node& operator=(const timer_node&) = delete;

    timer_node* parent{};
    timer_node* left{};
    timer_node* right{};
    std::chrono::steady_clock::time_point deadline;
    void (*fn)(void* x) noexcept { nullptr };
    void* x{ nullptr };
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