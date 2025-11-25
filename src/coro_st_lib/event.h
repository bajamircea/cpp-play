#pragma once

#include "context.h"

#include "../cpp_util_lib/intrusive_list.h"

#include <coroutine>

namespace coro_st
{
  class event
  {
  public:
    class [[nodiscard]] event_wait_task
    {
      friend class event;

      class [[nodiscard]] awaiter
      {
        friend class event;

        context& ctx_;
        std::coroutine_handle<> parent_handle_;
        event& evt_;
        awaiter* next_waiting_{ nullptr };
        awaiter* prev_waiting_{ nullptr };
        std::optional<stop_callback<callback>> parent_stop_cb_;

      public:
        awaiter(context& ctx, event& evt) noexcept :
          ctx_{ ctx },
          parent_handle_{},
          evt_{ evt },
          next_waiting_{ nullptr },
          prev_waiting_{ nullptr },
          parent_stop_cb_{ std::nullopt }
        {
        }

        awaiter(const awaiter&) = delete;
        awaiter& operator=(const awaiter&) = delete;

        [[nodiscard]] constexpr bool await_ready() const noexcept
        {
          return false;
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
          parent_handle_ = handle;
          enqueue_wait_node();
        }

        constexpr void await_resume() const noexcept
        {
        }

        std::exception_ptr get_result_exception() const noexcept
        {
          return {};
        }

        void start_as_chain_root() noexcept
        {
          enqueue_wait_node();
        }

      private:
        void enqueue_wait_node() noexcept
        {
          evt_.wait_list_.push_back(this);
          parent_stop_cb_.emplace(
            ctx_.get_stop_token(),
            make_member_callback<&awaiter::on_cancel>(this));
        }

        void on_event() noexcept
        {
          parent_stop_cb_.reset();
          evt_.wait_list_.remove(this);

          if (parent_handle_)
          {
            ctx_.schedule_coroutine_resume(parent_handle_);
            return;
          }

          ctx_.schedule_continuation();
        }

        void on_cancel() noexcept
        {
          parent_stop_cb_.reset();
          evt_.wait_list_.remove(this);
          ctx_.schedule_cancellation();
        }
      };

      struct [[nodiscard]] work
      {
        event* evt_;

        work(event& evt) noexcept :
          evt_{ &evt }
        {
        }

        work(const work&) = delete;
        work& operator=(const work&) = delete;
        work(work&&) noexcept = default;
        work& operator=(work&&) noexcept = default;

        [[nodiscard]] awaiter get_awaiter(context& ctx) noexcept
        {
          return {ctx, *evt_};
        }
      };

    private:
      work work_;

    public:
      event_wait_task(event& evt) noexcept :
        work_{ evt }
      {
      }

      event_wait_task(const event_wait_task&) = delete;
      event_wait_task& operator=(const event_wait_task&) = delete;

      [[nodiscard]] work get_work() noexcept
      {
        return std::move(work_);
      }
    };

private:
  using wait_list = cpp_util::intrusive_list<
    event_wait_task::awaiter,
    &event_wait_task::awaiter::next_waiting_,
    &event_wait_task::awaiter::prev_waiting_>;

  wait_list wait_list_;

public:
    bool notify_one() noexcept
    {
      if (wait_list_.empty())
      {
        return false;
      }
      wait_list_.front()->on_event();
      return true;
    }

    size_t notify_all() noexcept
    {
      size_t count = 0;
      // in a multithreaded solution where a lock is required
      // on the wait_list_, it might be more efficient to bulk
      // schedule them rather than notify one by one in a loop
      while (notify_one())
      {
        ++count;
      }
      return count;
    }

    [[nodiscard]] event_wait_task async_wait() noexcept
    {
      return event_wait_task{*this};
    }
  };
}
