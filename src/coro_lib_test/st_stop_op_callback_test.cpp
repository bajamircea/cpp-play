#include "../test_lib/test.h"

#include "../coro_lib/st_stop_op_callback.h"

namespace
{
  class Op
  {
    coro::st::stop_token token_;
    coro::st::stop_op_callback<Op> cb_{};
    int cancel_count_{};

    void cancel_op() noexcept
    {
      ++cancel_count_;
    }

  public:
    explicit Op(coro::st::stop_token token) noexcept :
      token_{ token }
    {
    }

    void start() noexcept
    {
      cb_.enable(token_, &Op::cancel_op, this);
    }

    void stop() noexcept
    {
      cb_.disable();
    }

    int cancel_count() const
    {
      return cancel_count_;
    }
  };

  TEST(st_stop_op_callback_simple)
  {
    coro::st::stop_source source;

    Op op{ source.get_token() };

    op.start();

    source.request_stop();

    op.stop();

    ASSERT_EQ(1, op.cancel_count());
  }
} // anonymous namespace