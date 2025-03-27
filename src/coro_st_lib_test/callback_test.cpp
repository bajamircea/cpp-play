#include "../test_lib/test.h"

#include "../coro_st_lib/callback.h"

namespace
{
  TEST(callback_trivial)
  {
    coro_st::callback cb;

    bool called{ false };

    struct X
    {
      bool& called;

      void set_called() noexcept
      {
        called = true;
      }
    };
    
    X x{ called };

    cb = coro_st::make_callback<&X::set_called>(&x);

    ASSERT_FALSE(called);

    cb.invoke();

    ASSERT_TRUE(called);
  }
} // anonymous namespace