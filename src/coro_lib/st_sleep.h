#pragma once

#include "st.h"

#include <chrono>
#include <thread>

namespace coro::st
{
  struct ready_awaiter
  {
    context& ctx_;
    ready_node node_;

    ready_awaiter(context& ctx) :
      ctx_{ ctx }
    {
    }

    [[nodiscard]] constexpr bool await_ready() const noexcept
    {
      return false;
    }

    void await_suspend(std::coroutine_handle<> handle) noexcept
    {
      ctx_.hazmat_push_ready_node(node_, handle);
    }

    constexpr void await_resume() const noexcept
    {
    }
  };

  ready_awaiter doze(context& ctx)
  {
    return ready_awaiter(ctx);
  }

  struct sleep_awaiter
  {
    context& ctx_;
    timer_node node_;

    sleep_awaiter(context& ctx, std::chrono::steady_clock::time_point deadline) :
      ctx_{ ctx }
    {
      node_.deadline = deadline;
    }

    [[nodiscard]] constexpr bool await_ready() const noexcept
    {
      return false;
    }

    void await_suspend(std::coroutine_handle<> handle) noexcept
    {
      ctx_.hazmat_insert_timer_node(node_, handle);
    }

    constexpr void await_resume() const noexcept
    {
    }
  };

  sleep_awaiter sleep(context& ctx, std::chrono::steady_clock::duration sleep_duration)
  {
    return sleep_awaiter(ctx, std::chrono::steady_clock::now() + sleep_duration);
  }
}
