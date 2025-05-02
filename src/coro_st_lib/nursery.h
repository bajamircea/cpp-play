#pragma once

#include "callback.h"
#include "context.h"
#include "coro_type_traits.h"
#include "stop_util.h"

#include "../cpp_util_lib/intrusive_list.h"

#include <coroutine>
#include <memory>
#include <type_traits>

namespace coro_st
{
  class nursery;

  namespace impl
  {
    struct nursery_child_node_base
    {
      nursery_child_node_base* next{ nullptr };
      nursery_child_node_base* prev{ nullptr };
  
      nursery_child_node_base() noexcept = default;
  
      nursery_child_node_base(const nursery_child_node_base&) = delete;
      nursery_child_node_base& operator=(const nursery_child_node_base&) = delete;

    protected:
      ~nursery_child_node_base() = default;
    };
  
    using nursery_children_list = cpp_util::intrusive_list<
      nursery_child_node_base,
      &nursery_child_node_base::next,
      &nursery_child_node_base::prev>;

    struct nursery_awaiter_shared_data
    {
      context& parent_ctx_;
      std::coroutine_handle<> parent_handle_;
      stop_source nursery_stop_source_;
      std::optional<stop_callback<callback>> stop_cb_;
      size_t pending_count_;
      std::exception_ptr exception_;
      impl::nursery_children_list children_;

      nursery_awaiter_shared_data(context& parent_ctx) noexcept :
        parent_ctx_{ parent_ctx },
        parent_handle_{},
        nursery_stop_source_{},
        stop_cb_{},
        pending_count_{ 0 },
        exception_{},
        children_{}
      {
      }

      nursery_awaiter_shared_data(const nursery_awaiter_shared_data&) = delete;
      nursery_awaiter_shared_data& operator=(const nursery_awaiter_shared_data&) = delete;

      void init_parent_cancellation_callback() noexcept
      {
        stop_cb_.emplace(
          parent_ctx_.get_stop_token(),
          make_member_callback<&nursery_awaiter_shared_data::on_parent_cancel>(this));
      }

      void on_shared_continue() noexcept
      {
        stop_cb_ = std::nullopt;

        if (parent_ctx_.get_stop_token().stop_requested())
        {
          parent_ctx_.schedule_cancellation_callback();
          return;
        }

        if (parent_handle_)
        {
          parent_ctx_.schedule_coroutine_resume(parent_handle_);
          return;
        }

        parent_ctx_.schedule_continuation_callback();
      }

      void on_parent_cancel() noexcept
      {
        stop_cb_ = std::nullopt;
        nursery_stop_source_.request_stop();
      }
    };

    template<is_co_work CoWork>
    struct nursery_initial_node : public nursery_child_node_base
    {
      nursery_awaiter_shared_data& shared_data_;
      chain_context chain_ctx_;
      context ctx_;
      co_work_awaiter_t<CoWork> co_awaiter_;

      nursery_initial_node(
        nursery_awaiter_shared_data& shared_data,
        CoWork& co_work
      ) :
        shared_data_{ shared_data },
        chain_ctx_{
          shared_data_.nursery_stop_source_.get_token(),
          make_member_callback<&nursery_initial_node::on_continue>(this),
          make_member_callback<&nursery_initial_node::on_cancel>(this),
          },
        ctx_{ shared_data_.parent_ctx_, chain_ctx_ },
        co_awaiter_{ co_work.get_awaiter(ctx_) }
      {
      }

      nursery_initial_node(const nursery_initial_node&) = delete;
      nursery_initial_node& operator=(const nursery_initial_node&) = delete;

      void on_continue() noexcept
      {
        if (!shared_data_.parent_ctx_.get_stop_token().stop_requested())
        {
          if (!shared_data_.exception_)
          {
            std::exception_ptr e = co_awaiter_.get_result_exception();
            if (e)
            {
              shared_data_.exception_ = e;
              shared_data_.nursery_stop_source_.request_stop();
            }
          }
        }

        complete_and_self_destroy();
      }

      void on_cancel() noexcept
      {
        complete_and_self_destroy();
      }

      void complete_and_self_destroy() noexcept
      {
        // take a copy of the members we use
        auto shared_data_local = &shared_data_;

        std::unique_ptr<nursery_initial_node> self_destruct{ this };

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
    impl::nursery_awaiter_shared_data* impl_{ nullptr };

  public:
    nursery() noexcept = default;

    nursery(const nursery&) noexcept = delete;
    nursery& operator=(const nursery&) noexcept = delete;

  private:
    template<is_co_task CoTask>
    class [[nodiscard]] nursery_run_task
    {
      using CoWork = co_task_work_t<CoTask>;
      using CoAwaiter = co_task_awaiter_t<CoTask>;

      nursery& nursery_;
      CoWork co_work_;
    public:
      nursery_run_task(
        nursery& nursery,
        CoTask& co_task
      ) noexcept :
        nursery_{ nursery },
        co_work_{ co_task.get_work() }
      {
      }

      nursery_run_task(const nursery_run_task&) = delete;
      nursery_run_task& operator=(const nursery_run_task&) = delete;

    private:
      class [[nodiscard]] awaiter
      {
        nursery& nursery_;
        impl::nursery_awaiter_shared_data shared_data_;
        std::unique_ptr<impl::nursery_initial_node<CoWork>> initial_unstarted_work_;

      public:
        awaiter(
          nursery& nursery,
          context& parent_ctx,
          CoWork& co_work
        ) :
          nursery_{ nursery },
          shared_data_{ parent_ctx },
          initial_unstarted_work_{
            std::make_unique<impl::nursery_initial_node<CoWork>>(
              shared_data_, co_work
            ) }
        {
          nursery_.impl_ = &shared_data_;
        }

        awaiter(const awaiter&) = delete;
        awaiter& operator=(const awaiter&) = delete;

        [[nodiscard]] constexpr bool await_ready() const noexcept
        {
          return false;
        }

        ~awaiter()
        {
          nursery_.impl_ = nullptr;
        }
      };

      struct [[nodiscard]] work
      {
        nursery& nursery_;
        CoWork co_work_;

        work(
          nursery& nursery,
          CoWork&& co_work_
        ) noexcept:
          nursery_{ nursery },
          co_work_{ std::move(co_work_) }
        {
        }

        work(const work&) = delete;
        work& operator=(const work&) = delete;
        work(work&&) noexcept = default;
        work& operator=(work&&) noexcept = default;

        [[nodiscard]] awaiter get_awaiter(context& ctx) noexcept
        {
          return awaiter(nursery_, ctx, co_work_);
        }
      };

    public:
      [[nodiscard]] work get_work() noexcept
      {
        return { nursery_, std::move(co_work_) };
      }
    };

  public:
    template<is_co_task CoTask>
    [[nodiscard]] nursery_run_task<CoTask> async_run(CoTask co_task)
    {
      static_assert(std::is_same_v<void, co_task_result_t<CoTask>>);
      return nursery_run_task<CoTask>{*this, co_task};
    }

    void request_stop() noexcept
    {

    }

    //void bind_and_spawn()
  };
}