#pragma once

#include "co.h"
#include "deferred_co.h"
#include "trampoline_co.h"
#include "st_context.h"
#include "st_type_traits.h"

#include <array>
#include <optional>
#include <type_traits>

// TODO: add cancellation

namespace coro::st
{
  template<typename T>
  struct wait_any_result
  {
    size_t index{};
    T value{};
  };

  template<typename T, size_t N>
  class [[nodiscard]] wait_any_awaiter
  {
  public:
    using co_return_type = wait_any_result<T>;

    struct chain_data
    {
    private:
      wait_any_awaiter& awaiter_;
      chain_context chain_ctx_;
      context ctx_;
      coro::trampoline_co<T> trampoline_;
    public:
      template<typename DeferredCoFn>
      chain_data(wait_any_awaiter& awaiter, DeferredCoFn&& co_fn) :
        awaiter_{ awaiter },
        chain_ctx_{ awaiter_.wait_stop_source_.get_token(), invoke_on_resume, this}, 
        ctx_{ awaiter_.parent_ctx_, chain_ctx_ },
        trampoline_([](context& ctx, DeferredCoFn& co_fn)
          -> coro::trampoline_co<T> {
            co_return co_await co_fn(ctx);
          }(ctx_, co_fn))
      {
        trampoline_.set_on_done_fn(invoke_on_done, this);
      }

      chain_data(const chain_data&) = delete;
      chain_data& operator=(const chain_data&) = delete;

      static void invoke_on_resume(void* x, std::coroutine_handle<> coroutine) noexcept
      {
        assert(x != nullptr);
        chain_data* self = reinterpret_cast<chain_data*>(x);
        self->on_resume(coroutine);
      }

      void on_resume(std::coroutine_handle<> coroutine) noexcept
      {
        if (chain_ctx_.get_stop_token().stop_requested())
        {
          on_done();
          return;
        }
        coroutine.resume();
      }

      static void invoke_on_done(void* x) noexcept
      {
        assert(x != nullptr);
        chain_data* self = reinterpret_cast<chain_data*>(x);
        return self->on_done();
      }

      void on_done() noexcept
      {
        if (!chain_ctx_.get_stop_token().stop_requested())
        {
          if (N == awaiter_.result_index_)
          {
            size_t this_index = this - awaiter_.children_chain_data_;
            awaiter_.result_index_ = this_index;
          }
        }
        --awaiter_.pending_count_;
        if (0 == awaiter_.pending_count_)
        {
          awaiter_.parent_ctx_.push_ready_node(awaiter_.node_, awaiter_.parent_handle_);
        }
      }

      void resume()
      {
        trampoline_.resume();
      }

      T get_result()
      {
        return trampoline_.get_result();
      }
    };

  private:
    context& parent_ctx_;
    ready_node node_;
    stop_source wait_stop_source_;
    std::coroutine_handle<> parent_handle_;
    size_t pending_count_;
    size_t result_index_;
    chain_data children_chain_data_[N];

  public:
    template<typename... DeferredCoFn>
    wait_any_awaiter(context& ctx, DeferredCoFn&&... co_fns) noexcept :
      parent_ctx_{ ctx },
      node_{},
      wait_stop_source_{},
      parent_handle_{},
      pending_count_{ N + 1 },
      result_index_{ N },
      children_chain_data_{{*this, std::forward<DeferredCoFn>(co_fns)}...}
    {
      static_assert(N == sizeof...(co_fns));
    }

    wait_any_awaiter(const wait_any_awaiter&) = delete;
    wait_any_awaiter& operator=(const wait_any_awaiter&) = delete;

  private:
    bool await_suspend_impl(std::coroutine_handle<> handle) noexcept
    {
      parent_handle_ = handle;
      // TODO: add cancellation

      for(auto& child: children_chain_data_)
      {
        child.resume();
      }

      --pending_count_;
      if (0 == pending_count_)
      {
        return false;
      }
      return true;
    }

    // TODO add back const
    co_return_type await_resume_impl() /*const*/
    {
      assert(result_index_ != N);
      return co_return_type{
        .index=result_index_,
        .value=children_chain_data_[result_index_].get_result()
      };
    }

    class [[nodiscard]] awaiter
    {
      wait_any_awaiter& impl_;

    public:
      awaiter(wait_any_awaiter& impl) noexcept : impl_{ impl }
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
        return impl_.await_suspend_impl(handle);
      }

      co_return_type await_resume() const
      {
        return impl_.await_resume_impl();
      }
    };

    friend awaiter;

    [[nodiscard]] friend awaiter operator co_await(wait_any_awaiter x) noexcept
    {
      return { x };
    }
  };

  template<is_deferred_context_co... DeferredCoFn>
  [[nodiscard]] auto async_wait_any(context& ctx, DeferredCoFn&&... co_fns)
    -> wait_any_awaiter<
        std::common_type_t<
          deferred_context_co_return_type<DeferredCoFn>...>,
        sizeof...(co_fns)>
  {
    return { ctx , std::forward<DeferredCoFn>(co_fns)...};
  }
}