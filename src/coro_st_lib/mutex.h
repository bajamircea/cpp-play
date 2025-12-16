#pragma once

#include "context.h"

#include "../cpp_util_lib/intrusive_list.h"

#include <cassert>
#include <coroutine>

namespace coro_st
{
  class mutex
  {
  public:
    class [[nodiscard]] mutex_lock_task
    {
      friend class mutex;

      class [[nodiscard]] awaiter
      {
        friend class mutex;

        class [[nodiscard]] scoped_lock
        {
          friend class awaiter;

          mutex& mtx_;
          explicit scoped_lock(mutex& mtx) noexcept : mtx_{ mtx }
          {
            assert(mtx_.locked_);
          }

        public:
          scoped_lock(const scoped_lock&) = delete;
          scoped_lock& operator=(const scoped_lock&) = delete;

          ~scoped_lock()
          {
            mtx_.unlock();
          }
        };

        context& ctx_;
        std::coroutine_handle<> parent_handle_;
        mutex& mtx_;
        awaiter* next_waiting_{ nullptr };
        awaiter* prev_waiting_{ nullptr };
        std::optional<stop_callback<callback>> parent_stop_cb_;

      public:
        awaiter(context& ctx, mutex& mtx) noexcept :
          ctx_{ ctx },
          parent_handle_{},
          mtx_{ mtx },
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

        bool await_suspend(std::coroutine_handle<> handle) noexcept
        {
          parent_handle_ = handle;
          if (!mtx_.locked_)
          {
            if (ctx_.get_stop_token().stop_requested())
            {
              ctx_.invoke_stopped();
              return true;
            }
            mtx_.locked_ = true;
            return false;
          }
          enqueue_wait_node();
          return true;
        }

        scoped_lock await_resume() noexcept
        {
          return scoped_lock{ mtx_ };
        }

        std::exception_ptr get_result_exception() const noexcept
        {
          return {};
        }

        void start() noexcept
        {
          if (!mtx_.locked_)
          {
            if (ctx_.get_stop_token().stop_requested())
            {
              ctx_.invoke_stopped();
              return;
            }
            mtx_.locked_ = true;
            ctx_.invoke_result_ready();
            return;
          }
          enqueue_wait_node();
        }

      private:
        void enqueue_wait_node() noexcept
        {
          mtx_.wait_list_.push_back(this);
          parent_stop_cb_.emplace(
            ctx_.get_stop_token(),
            make_member_callback<&awaiter::on_cancel>(this));
        }

        void on_event() noexcept
        {
          parent_stop_cb_.reset();
          mtx_.wait_list_.remove(this);

          if (parent_handle_)
          {
            ctx_.schedule_coroutine_resume(parent_handle_);
            return;
          }

          ctx_.schedule_result_ready();
        }

        void on_cancel() noexcept
        {
          parent_stop_cb_.reset();
          mtx_.wait_list_.remove(this);
          ctx_.schedule_stopped();
        }
      };

      struct [[nodiscard]] work
      {
        mutex* mtx_;

        work(mutex& mtx) noexcept :
          mtx_{ &mtx }
        {
        }

        work(const work&) = delete;
        work& operator=(const work&) = delete;
        work(work&&) noexcept = default;
        work& operator=(work&&) noexcept = default;

        [[nodiscard]] awaiter get_awaiter(context& ctx) noexcept
        {
          return {ctx, *mtx_};
        }
      };

    private:
      work work_;

    public:
      mutex_lock_task(mutex& mtx) noexcept :
        work_{ mtx }
      {
      }

      mutex_lock_task(const mutex_lock_task&) = delete;
      mutex_lock_task& operator=(const mutex_lock_task&) = delete;

      [[nodiscard]] work get_work() noexcept
      {
        return std::move(work_);
      }
    };

  private:
    using wait_list = cpp_util::intrusive_list<
      mutex_lock_task::awaiter,
      &mutex_lock_task::awaiter::next_waiting_,
      &mutex_lock_task::awaiter::prev_waiting_>;

    wait_list wait_list_;
    bool locked_{false};

    void unlock() noexcept
    {
      assert(locked_);
      if (wait_list_.empty())
      {
        locked_ = false;
        return;
      }
      wait_list_.front()->on_event();
    }

public:
    [[nodiscard]] mutex_lock_task async_lock() noexcept
    {
      return mutex_lock_task{*this};
    }

    bool is_locked() const noexcept
    {
      return locked_;
    }
  };
}
