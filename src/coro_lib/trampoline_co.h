#pragma once

#include "unique_coroutine_handle.h"
#include "promise_base.h"

#include <cassert>
#include <coroutine>
#include <utility>

namespace coro
{
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

      std::suspend_always initial_suspend() noexcept
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

    void resume() const
    {
      return unique_child_coro_.get().resume();      
    }

    bool done() const noexcept
    {
      return unique_child_coro_.get().done();
    }

    T get_result()
    {
      return unique_child_coro_.get().promise().get_result();
    }
  };
}