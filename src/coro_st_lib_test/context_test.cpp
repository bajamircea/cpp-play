#include "../test_lib/test.h"

#include "../coro_st_lib/context.h"

namespace
{
  TEST(context_trivial)
  {
    coro_st::stop_source stop_source;

    coro_st::ready_queue q;
    coro_st::timer_heap h;

    coro_st::event_loop_context event_loop_context{ q, h };

    struct completion_flags
    {
      bool result_ready{ false };
      bool stopped{ false };
    };
    completion_flags cf;

    coro_st::context x{
      event_loop_context,
      stop_source.get_token(),
      coro_st::make_function_completion<
        +[](completion_flags& x) noexcept {
          x.result_ready = true;
        },
        +[](completion_flags& x) noexcept {
          x.stopped = true;
        }
        >(cf)
    };

    ASSERT_TRUE(q.empty());
    ASSERT_TRUE(h.empty());

    x.schedule_result_ready();

    ASSERT_FALSE(q.empty());
    ASSERT_TRUE(h.empty());

    auto pq = q.pop();
    ASSERT_EQ(pq, &x.get_chain_node());

    pq->cb.invoke();

    ASSERT_TRUE(cf.result_ready);
    ASSERT_TRUE(q.empty());
  }
} // anonymous namespace