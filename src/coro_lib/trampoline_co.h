#pragma once

#include "unique_coroutine_handle.h"
#include "promise_base.h"

#include <cassert>
#include <coroutine>
#include <utility>

namespace coro
{
  using OnTrampolineDoneFnPtr = void (*)(void* x) noexcept;

  struct coroutine_frame_abi
  {
    void (*resume)(void*);
    void (*destroy)(void*);
  };

  class trampoline_frame : public coroutine_frame_abi
  {
    void* no_parent_;
    OnTrampolineDoneFnPtr on_done_fn_{ nullptr };
    void* x_{ nullptr };
  public:
    trampoline_frame(OnTrampolineDoneFnPtr on_done_fn, void* x) noexcept :
      coroutine_frame_abi{ .resume=resume_impl, .destroy=destroy_impl },
      no_parent_{ nullptr },
      on_done_fn_{ on_done_fn },
      x_{ x }
    {
      assert(nullptr != on_done_fn);
    }

    trampoline_frame(const trampoline_frame&) = delete;
    trampoline_frame& operator=(const trampoline_frame&) = delete;

  private:
    static void resume_impl(void* frame_ptr) noexcept
    {
      assert(frame_ptr != nullptr);
      trampoline_frame* self = reinterpret_cast<trampoline_frame*>(frame_ptr);
      self->on_done_fn_(self->x_);
    }

    static void destroy_impl(void*) noexcept
    {
      std::terminate();
    }
  };

  template<typename T>
  class [[nodiscard]] trampoline_co
  {
  public:
    class promise_type : public promise_base<T>
    {
      friend trampoline_co;

      OnTrampolineDoneFnPtr on_done_fn_{ nullptr };
      void* x_{ nullptr };

    public:
      promise_type() noexcept = default;

      promise_type(const promise_type&) = delete;
      promise_type& operator=(const promise_type&) = delete;

      trampoline_co get_return_object() noexcept
      {
        return { std::coroutine_handle<promise_type>::from_promise(*this) };
      }

      std::suspend_always initial_suspend() noexcept
      {
        return {};
      }

      struct final_awaiter
      {
        [[nodiscard]] constexpr bool await_ready() const noexcept
        {
          return false;
        }

        void await_suspend(std::coroutine_handle<promise_type> child_coro) noexcept
        {
          promise_type& promise= child_coro.promise();
          OnTrampolineDoneFnPtr on_done_fn = promise.on_done_fn_;
          void* x = promise.x_;
          on_done_fn(x);
        }

        [[noreturn]] constexpr void await_resume() const noexcept
        {
          std::unreachable();
          //std::terminate();
        }
      };

      final_awaiter final_suspend() noexcept
      {
        assert(nullptr != on_done_fn_);
        return {};
      }
    };

  private:
    unique_coroutine_handle<promise_type> unique_child_coro_;

    trampoline_co(std::coroutine_handle<promise_type> child_coro) noexcept :
      unique_child_coro_{ child_coro }
    {
    }

  public:
    trampoline_co(const trampoline_co&) = delete;
    trampoline_co& operator=(const trampoline_co&) = delete;

    void set_on_done_fn(OnTrampolineDoneFnPtr on_done_fn, void* x) noexcept
    {
      assert(nullptr != on_done_fn);
      auto& promise = unique_child_coro_.get().promise();
      promise.on_done_fn_ = on_done_fn;
      promise.x_ = x;
    }

    void resume() const
    {
      assert(nullptr != unique_child_coro_.get().promise().on_done_fn_);
      return unique_child_coro_.get().resume();
    }

    bool done() const noexcept
    {
      assert(nullptr != unique_child_coro_.get().promise().on_done_fn_);
      return unique_child_coro_.get().done();
    }

    T get_result() const
    {
      assert(nullptr != unique_child_coro_.get().promise().on_done_fn_);
      return unique_child_coro_.get().promise().get_result();
    }
  };
}