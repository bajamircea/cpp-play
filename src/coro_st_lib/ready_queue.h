#pragma once

#include "../cpp_util_lib/intrusive_queue.h"

#include <coroutine>

namespace coro_st
{
  struct ready_node
  {
    ready_node() noexcept = default;

    ready_node(const ready_node&) = delete;
    ready_node& operator=(const ready_node&) = delete;

    ready_node* next{};
    void (*fn)(void* x) noexcept { nullptr };
    void* x{ nullptr };
  };

  using ready_queue = cpp_util::intrusive_queue<ready_node, &ready_node::next>;
}