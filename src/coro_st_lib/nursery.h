#pragma once

#include "call_capture.h"
#include "callback.h"
#include "context.h"
#include "coro_type_traits.h"
#include "stop_util.h"

#include <cassert>
#include <coroutine>
#include <memory>
#include <type_traits>

namespace coro_st
{
  class nursery;

  namespace impl
  {
    struct nursery_awaiter_shared_data
    {
      enum class result_state
      {
        completion, // includes case of nursery::request_stop
        cancellation,
      };

      context& parent_ctx_;
      std::coroutine_handle<> parent_handle_;
      std::optional<stop_callback<callback>> parent_stop_cb_;
      stop_source children_stop_source_;
      size_t pending_count_;
      std::exception_ptr exception_;
      result_state result_state_{ result_state::completion };

      nursery_awaiter_shared_data(context& parent_ctx) noexcept :
        parent_ctx_{ parent_ctx },
        parent_handle_{},
        parent_stop_cb_{},
        children_stop_source_{},
        pending_count_{ 0 },
        exception_{},
        result_state_{ result_state::completion }
      {
      }

      nursery_awaiter_shared_data(const nursery_awaiter_shared_data&) = delete;
      nursery_awaiter_shared_data& operator=(const nursery_awaiter_shared_data&) = delete;

      void init_parent_cancellation_callback() noexcept
      {
        parent_stop_cb_.emplace(
          parent_ctx_.get_stop_token(),
          make_member_callback<&nursery_awaiter_shared_data::on_parent_cancel>(this));
      }

      void on_shared_continue() noexcept
      {
        parent_stop_cb_.reset();

        if (result_state::cancellation == result_state_)
        {
          parent_ctx_.schedule_cancellation();
          return;
        }

        if (parent_handle_)
        {
          parent_ctx_.schedule_coroutine_resume(parent_handle_);
          return;
        }

        parent_ctx_.schedule_continuation();
      }

      void on_parent_cancel() noexcept
      {
        parent_stop_cb_.reset();
        result_state_ = result_state::cancellation;
        children_stop_source_.request_stop();
      }
    };

    template<is_co_work CoWork>
    struct nursery_initial_child
    {
      nursery_awaiter_shared_data& shared_data_;
      chain_context chain_ctx_;
      context ctx_;
      co_work_awaiter_t<CoWork> co_awaiter_;

      nursery_initial_child(
        nursery_awaiter_shared_data& shared_data,
        CoWork& co_work
      ) :
        shared_data_{ shared_data },
        chain_ctx_{
          shared_data_.children_stop_source_.get_token(),
          make_member_callback<&nursery_initial_child::on_continue>(this),
          make_member_callback<&nursery_initial_child::on_cancel>(this),
          },
        ctx_{ shared_data_.parent_ctx_, chain_ctx_ },
        co_awaiter_{ co_work.get_awaiter(ctx_) }
      {
      }

      nursery_initial_child(const nursery_initial_child&) = delete;
      nursery_initial_child& operator=(const nursery_initial_child&) = delete;

      void on_continue() noexcept
      {
        if (nursery_awaiter_shared_data::result_state::completion == shared_data_.result_state_)
        {
          if (!shared_data_.exception_)
          {
            std::exception_ptr e = co_awaiter_.get_result_exception();
            if (e)
            {
              shared_data_.exception_ = e;
              shared_data_.children_stop_source_.request_stop();
            }
          }
        }

        complete_and_self_destroy();
      }

      void on_cancel() noexcept
      {
        if (nursery_awaiter_shared_data::result_state::completion == shared_data_.result_state_)
        {
          if (!shared_data_.children_stop_source_.stop_requested())
          {
            shared_data_.result_state_ = nursery_awaiter_shared_data::result_state::cancellation;
            shared_data_.children_stop_source_.request_stop();
          }
        }

        complete_and_self_destroy();
      }

      void start() noexcept
      {
        ++shared_data_.pending_count_;
        co_awaiter_.start();
      }

      void complete_and_self_destroy() noexcept
      {
        // take a copy of the members we use
        auto shared_data_local = &shared_data_;

        std::unique_ptr<nursery_initial_child> self_destruct{ this };

        --shared_data_local->pending_count_;
        if (0 != shared_data_local->pending_count_)
        {
          return;
        }
        self_destruct.reset();

        shared_data_local->on_shared_continue();
      }
    };

    template<typename Fn, typename... Args>
    struct nursery_spawn_child
    {
      using Capture = call_capture<Fn, Args...>;
      using Awaiter = co_task_awaiter_t<
        typename Capture::result_type>;

      Capture lifetime_capture_;
      nursery_awaiter_shared_data& shared_data_;
      chain_context chain_ctx_;
      context ctx_;
      Awaiter co_awaiter_;

      template<typename Fn2, typename... Args2>
      nursery_spawn_child(
        nursery_awaiter_shared_data& shared_data,
        Fn2&& fn,
        Args2&&... args
      ) :
        lifetime_capture_{
          std::forward<Fn2>(fn),
          std::forward<Args2>(args)... },
        shared_data_{ shared_data },
        chain_ctx_{
          shared_data_.children_stop_source_.get_token(),
          make_member_callback<&nursery_spawn_child::on_continue>(this),
          make_member_callback<&nursery_spawn_child::on_cancel>(this),
          },
        ctx_{ shared_data_.parent_ctx_, chain_ctx_ },
        co_awaiter_{ lifetime_capture_().get_work().get_awaiter(ctx_) }
      {
      }

      nursery_spawn_child(const nursery_spawn_child&) = delete;
      nursery_spawn_child& operator=(const nursery_spawn_child&) = delete;

      void on_continue() noexcept
      {
        if (nursery_awaiter_shared_data::result_state::completion == shared_data_.result_state_)
        {
          if (!shared_data_.exception_)
          {
            std::exception_ptr e = co_awaiter_.get_result_exception();
            if (e)
            {
              shared_data_.exception_ = e;
              shared_data_.children_stop_source_.request_stop();
            }
          }
        }

        complete_and_self_destroy();
      }

      void on_cancel() noexcept
      {
        if (nursery_awaiter_shared_data::result_state::completion == shared_data_.result_state_)
        {
          if (!shared_data_.children_stop_source_.stop_requested())
          {
            shared_data_.result_state_ = nursery_awaiter_shared_data::result_state::cancellation;
            shared_data_.children_stop_source_.request_stop();
          }
        }

        complete_and_self_destroy();
      }

      void start() noexcept
      {
        ++shared_data_.pending_count_;
        co_awaiter_.start();
      }

      void complete_and_self_destroy() noexcept
      {
        // take a copy of the members we use
        auto shared_data_local = &shared_data_;

        std::unique_ptr<nursery_spawn_child> self_destruct{ this };

        --shared_data_local->pending_count_;
        if (0 != shared_data_local->pending_count_)
        {
          return;
        }
        self_destruct.reset();

        shared_data_local->on_shared_continue();
      }
    };
  }

  class nursery
  {
  public:
    template<is_co_task CoTask>
    class [[nodiscard]] nursery_run_task
    {
      using CoWork = co_task_work_t<CoTask>;
      using CoAwaiter = co_task_awaiter_t<CoTask>;

    private:
      class [[nodiscard]] awaiter
      {
        nursery& nursery_;
        impl::nursery_awaiter_shared_data shared_data_;
        std::unique_ptr<impl::nursery_initial_child<CoWork>> initial_unstarted_work_;

      public:
        awaiter(
          context& parent_ctx,
          nursery& nursery,
          CoWork& co_work
        ) :
          nursery_{ nursery },
          shared_data_{ parent_ctx },
          initial_unstarted_work_{
            std::make_unique<impl::nursery_initial_child<CoWork>>(
              shared_data_, co_work
            ) }
        {
          assert(nullptr == nursery_.impl_);
          nursery_.impl_ = &shared_data_;
        }

        awaiter(const awaiter&) = delete;
        awaiter& operator=(const awaiter&) = delete;

        ~awaiter()
        {
          nursery_.impl_ = nullptr;
        }

        [[nodiscard]] constexpr bool await_ready() const noexcept
        {
          return false;
        }

        bool await_suspend(std::coroutine_handle<> handle) noexcept
        {
          shared_data_.parent_handle_ = handle;

          shared_data_.pending_count_ = 1;
          shared_data_.init_parent_cancellation_callback();

          initial_unstarted_work_.release()->start();

          --shared_data_.pending_count_;
          if (0 != shared_data_.pending_count_)
          {
            return true;
          }

          shared_data_.parent_stop_cb_.reset();

          if (impl::nursery_awaiter_shared_data::result_state::cancellation == shared_data_.result_state_)
          {
            shared_data_.parent_ctx_.schedule_cancellation();
            return true;
          }

          return false;
        }

        void await_resume() const
        {
          assert(impl::nursery_awaiter_shared_data::result_state::completion == shared_data_.result_state_);
          if (shared_data_.exception_)
          {
            std::rethrow_exception(shared_data_.exception_);
          }
        }

        std::exception_ptr get_result_exception() const noexcept
        {
          return shared_data_.exception_;
        }

        void start() noexcept
        {
          shared_data_.pending_count_ = 1;
          shared_data_.init_parent_cancellation_callback();

          initial_unstarted_work_.release()->start();

          --shared_data_.pending_count_;
          if (0 != shared_data_.pending_count_)
          {
            return;
          }

          shared_data_.parent_stop_cb_.reset();

          if (impl::nursery_awaiter_shared_data::result_state::cancellation == shared_data_.result_state_)
          {
            shared_data_.parent_ctx_.invoke_cancellation();
            return;
          }

          shared_data_.parent_ctx_.invoke_continuation();
        }
      };

      struct [[nodiscard]] work
      {
        nursery* nursery_;
        CoWork co_work_;

        work(
          nursery& nursery,
          CoWork&& co_work_
        ) noexcept:
          nursery_{ &nursery },
          co_work_{ std::move(co_work_) }
        {
        }

        work(const work&) = delete;
        work& operator=(const work&) = delete;
        work(work&&) noexcept = default;
        work& operator=(work&&) noexcept = default;

        [[nodiscard]] awaiter get_awaiter(context& ctx) noexcept
        {
          return {ctx, *nursery_, co_work_};
        }
      };

    private:
      work work_;

    public:
      nursery_run_task(
        nursery& nursery,
        CoTask& co_task
      ) noexcept :
        work_{ nursery, co_task.get_work() }
      {
      }

      nursery_run_task(const nursery_run_task&) = delete;
      nursery_run_task& operator=(const nursery_run_task&) = delete;

      [[nodiscard]] work get_work() noexcept
      {
        return std::move(work_);
      }
    };

  private:
    impl::nursery_awaiter_shared_data* impl_{ nullptr };

  public:
    nursery() noexcept = default;

    nursery(const nursery&) noexcept = delete;
    nursery& operator=(const nursery&) noexcept = delete;

    template<is_co_task CoTask>
    [[nodiscard]] nursery_run_task<CoTask> async_run(CoTask co_task)
    {
      static_assert(std::is_same_v<void, co_task_result_t<CoTask>>);
      return nursery_run_task<CoTask>{*this, co_task};
    }

    void request_stop() noexcept
    {
      assert(nullptr != impl_);
      impl_->children_stop_source_.request_stop();
    }

    template<typename Fn, typename... Args>
    void spawn_child(Fn&& fn, Args&&... args)
    {
      using CaptureResult = typename call_capture<Fn, Args...>::result_type;
      static_assert(is_co_task<CaptureResult>);
      static_assert(std::is_same_v<
        void,
        co_task_result_t<CaptureResult>>);

      assert(nullptr != impl_);

      auto spawn_unstarted_work_ =
        std::make_unique<
          impl::nursery_spawn_child<Fn, Args...>>(
            *impl_,
            std::forward<Fn>(fn),
            std::forward<Args>(args)...
        );

        spawn_unstarted_work_.release()->start();
    }
  };
}