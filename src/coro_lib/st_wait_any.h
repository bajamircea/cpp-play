#pragma once

#include "co.h"
#include "deferred_co.h"
#include "trampoline_co.h"
#include "st_context.h"
#include "st_type_traits.h"

#include <type_traits>

namespace coro::st
{
  // TODO create async_wait_any and async_wait_all
  template<typename T>
  struct wait_any_result
  {
    size_t index{};
    T value{};
  };

  template<typename Result>
  class [[nodiscard]] wait_any_awaiter
  {
  public:
    using co_return_type = Result;

    struct chain_data
    {
    private:
      context& ctx_;
      wait_any_awaiter& awaiter_;
      size_t index_;
    public:
      chain_data(context& ctx, wait_any_awaiter& awaiter, size_t index) :
        ctx_{ ctx }, awaiter_{ awaiter }, index_{ index }
      {
      }
    };

  private:
    context& ctx_;
    stop_source wait_stop_source_;
    ready_node node_;

  public:
    wait_any_awaiter(context& ctx) noexcept :
      ctx_{ ctx }
    {
    }

    wait_any_awaiter(const wait_any_awaiter&) = delete;
    wait_any_awaiter& operator=(const wait_any_awaiter&) = delete;

  private:
    void await_suspend_impl(std::coroutine_handle<> handle) noexcept
    {
      ctx_.push_ready_node(node_, handle);
    }

    Result await_resume_impl() const
    {
      return Result{.index=0, .value=0};
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

      void await_suspend(std::coroutine_handle<> handle) noexcept
      {
        return impl_.await_suspend_impl(handle);
      }

      Result await_resume() const
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
    -> wait_any_awaiter<wait_any_result<
        std::common_type_t<
          deferred_context_co_return_type<DeferredCoFn>...>>>
  {
    return { ctx };
  }

  //   

  //   custom_chain_context chain_ctx{
  //     main_stop_source.get_token(),
  //     [](std::coroutine_handle<> coroutine) noexcept {
  //       coroutine.resume();
  //     }
  //   };
  //   context main_ctx(runner_ctx, chain_ctx);

  //   using TrampolineType = coro::trampoline_co<deferred_context_co_return_type<DeferredCoFn>>;
  //   auto trampoline = [](context& ctx, DeferredCoFn& co_fn)
  //    -> TrampolineType {
  //     co_return co_await co_fn(ctx);
  //   };

  //   bool done{ false };
  //   OnTrampolineDoneFnPtr on_done = +[](void* x) noexcept {
  //     bool* p_done = reinterpret_cast<bool*>(x);
  //     *p_done = true;
  //   };
  //   auto main_co = trampoline(main_ctx, co_fn);
  //   main_co.set_fn(on_done, &done);
  //   main_co.resume();

  //   while (!done)
  //   {
  //     runner.do_work();
  //   }

  //   return main_co.get_result();
  // }
}