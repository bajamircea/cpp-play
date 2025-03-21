#pragma once

#include "trampoline_co.h"
#include "st_context.h"
#include "st_type_traits.h"
#include "st_stop_op_callback.h"

#include <type_traits>

namespace coro::st
{
  template<typename T>
  struct wait_any_result
  {
    size_t index{};
    T value{};
  };

  template<>
  struct wait_any_result<void>
  {
    size_t index{};
  };

  // TODO: revisit all the forward/move
  template<typename... DeferredCoFn>
  class [[nodiscard]] wait_any_awaitable
  {
    using T = std::common_type_t<
      context_callable_await_result_t<DeferredCoFn>...>;
    static constexpr size_t N = sizeof... (DeferredCoFn);
    using DeferredCoFnsTuple = std::tuple<DeferredCoFn...>;
  public:
    using co_return_type = wait_any_result<T>;

  private:
    context& parent_ctx_;
    DeferredCoFnsTuple co_fns_tuple_;

  public:
    template<typename... DeferredCoFn2>
    wait_any_awaitable(context& ctx, DeferredCoFn2&&... co_fns) noexcept :
      parent_ctx_{ ctx },
      co_fns_tuple_{ std::forward<DeferredCoFn2>(co_fns)... }
    {
      static_assert(N == sizeof... (DeferredCoFn2));
    }

    wait_any_awaitable(const wait_any_awaitable&) = delete;
    wait_any_awaitable& operator=(const wait_any_awaitable&) = delete;

  private:
    class [[nodiscard]] awaiter
    {
      struct chain_data
      {
      private:
        awaiter& awaiter_;
        chain_context chain_ctx_;
        context ctx_;
        coro::trampoline_co<T> trampoline_;
      public:
        template<typename DeferredCoFn2>
        chain_data(awaiter& awaiter, DeferredCoFn2&& co_fn) :
          awaiter_{ awaiter },
          chain_ctx_{ awaiter_.wait_stop_source_.get_token(), invoke_on_resume, this},
          ctx_{ awaiter_.parent_ctx_, chain_ctx_ },
          trampoline_([](context& ctx, DeferredCoFn2& co_fn)
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
              awaiter_.wait_stop_source_.request_stop();
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
  
        T get_result() const
        {
          return trampoline_.get_result();
        }
      };

      context& parent_ctx_;
      stop_op_callback<awaiter> stop_cb_;
      // TODO (low pri, see cppcoro) do I need the node?
      ready_node node_;
      stop_source wait_stop_source_;
      std::coroutine_handle<> parent_handle_;
      size_t pending_count_;
      size_t result_index_;
      chain_data children_chain_data_[N];

    public:
      template<typename... DeferredCoFn2>
      awaiter(context& parent_ctx, DeferredCoFn2&&... co_fns) noexcept :
        parent_ctx_{ parent_ctx },
        node_{},
        wait_stop_source_{},
        parent_handle_{},
        pending_count_{ N + 1 },
        result_index_{ N },
        children_chain_data_{{*this, std::forward<DeferredCoFn2>(co_fns)}...}
      {
        static_assert(N == sizeof...(co_fns));
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
  
        for(auto& child: children_chain_data_)
        {
          child.resume();
        }
  
        --pending_count_;
        if (0 == pending_count_)
        {
          return false;
        }
        stop_cb_.enable(parent_ctx_.get_stop_token(), &awaiter::cancel, this);
        return true;
      }

      co_return_type await_resume() const
      {
        assert(result_index_ != N);
        if constexpr (std::is_same_v<T, void>)
        {
          children_chain_data_[result_index_].get_result();
          return co_return_type{
            .index=result_index_
          };
        }
        else
        {
          return co_return_type{
            .index=result_index_,
            .value=children_chain_data_[result_index_].get_result()
          };
        }
      }

      void cancel() noexcept
      {
        stop_cb_.disable();
        wait_stop_source_.request_stop();
        parent_ctx_.push_ready_node(node_, std::coroutine_handle<>());
      }
    };

    template<std::size_t... I>
    awaiter make_awaiter(std::index_sequence<I...>)
    {
      return awaiter{ parent_ctx_, std::get<I>(co_fns_tuple_)... };
    }

  private:
    [[nodiscard]] friend awaiter operator co_await(wait_any_awaitable x) noexcept
    {
      return std::move(x).hazmat_get_awaiter();
    }

  public:
    [[nodiscard]] awaiter hazmat_get_awaiter() && noexcept
    {
      return make_awaiter(std::index_sequence_for<DeferredCoFn...>{});
    }
  };

  // template<typename... DeferredCoFn>
  // wait_any_awaitable(context& ctx, DeferredCoFn&&... co_fns)
  //   -> wait_any_awaitable<DeferredCoFn...>;

  template<is_context_callable_co... DeferredCoFn>
  [[nodiscard]] wait_any_awaitable<DeferredCoFn...>
    async_wait_any(context& ctx, DeferredCoFn&&... co_fns)
  {
    return wait_any_awaitable<DeferredCoFn...>{ ctx , std::forward<DeferredCoFn>(co_fns)... };
  }
}