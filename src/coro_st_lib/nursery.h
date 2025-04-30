#pragma once

#include "callback.h"
#include "context.h"
#include "coro_type_traits.h"
#include "stop_util.h"

#include "../cpp_util_lib/intrusive_list.h"

#include <coroutine>
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
  }

  class nursery
  {
    class nursery_run_awaiter;
    friend class nursery_run_awaiter;

    nursery_run_awaiter* awaiter_{ nullptr };

  public:
    nursery() noexcept = default;

    nursery(const nursery&) noexcept = delete;
    nursery& operator=(const nursery&) noexcept = delete;

  private:
    class nursery_run_awaiter
    {
      nursery& nursery_;
      context& parent_ctx_;
      impl::nursery_children_list children_;

    public:
      template<is_co_work CoWork>
      nursery_run_awaiter(
        nursery& nursery,
        context& parent_ctx,
        CoWork& /*co_work*/
      ) :
        nursery_{ nursery },
        parent_ctx_{ parent_ctx }
      {
        nursery_.awaiter_ = this;
      }

      nursery_run_awaiter(const nursery_run_awaiter&) = delete;
      nursery_run_awaiter& operator=(const nursery_run_awaiter&) = delete;

      ~nursery_run_awaiter()
      {
        nursery_.awaiter_ = nullptr;
      }
    };

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

        [[nodiscard]] nursery_run_awaiter get_awaiter(context& ctx) noexcept
        {
          return nursery_run_awaiter(nursery_, ctx, co_work_);
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