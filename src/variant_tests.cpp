#include "variant.h"

#include "test.h"

namespace
{
  using namespace variant;

  TEST(variant__use_with_int_bool)
  {
    using pib = ptr<int, bool>;

    struct detect_empty
    {
      explicit detect_empty() :
        called_{ false }
      {
      }

      void operator()(int &)
      {
        FAIL();
      }

      void operator()(bool &)
      {
        FAIL();
      }

      void operator()()
      {
        ASSERT_FALSE(called_);
        called_ = true;
      }

      bool called_;
    };

    struct detect_int
    {
      explicit detect_int(int & v) :
        v_{ v },
        called_{ false }
      {
      }

      void operator()(int & v)
      {
        v_ = v;
        ASSERT_FALSE(called_);
        called_ = true;
      }

      void operator()(bool &)
      {
        FAIL();
      }

      void operator()()
      {
        FAIL();
      }

      int & v_;
      bool called_;
    };

    struct detect_const_int
    {
      explicit detect_const_int(int & v) :
        v_{ v },
        called_{ false }
      {
      }

      void operator()(const int & v)
      {
        v_ = v;
        ASSERT_FALSE(called_);
        called_ = true;
      }

      void operator()(const bool &)
      {
        FAIL();
      }

      void operator()()
      {
        FAIL();
      }

      int & v_;
      bool called_;
    };

    struct detect_bool
    {
      explicit detect_bool(bool & v) :
        v_{ v },
        called_{ false }
      {
      }

      void operator()(int &)
      {
        FAIL();
      }

      void operator()(bool & v)
      {
        v_ = v;
        ASSERT_FALSE(called_);
        called_ = true;
      }

      void operator()()
      {
        FAIL();
      }

      bool & v_;
      bool called_;
    };

    {
      pib x;
      ASSERT(x.is_empty() && !x.is<int>() && !x.is<bool>());
      x.apply(detect_empty());
      ASSERT_THROW(x.at<int>(), std::bad_cast);
    }

    {
      pib x{ 42 };
      int actual_int = 0;
      ASSERT(!x.is_empty() && x.is<int>() && !x.is<bool>());
      x.apply(detect_int(actual_int));
      ASSERT_EQ(42, actual_int);
      ASSERT_EQ(42, x.at<int>());
      ASSERT_EQ(42, x.get<int>());
      ASSERT_THROW(x.at<bool>(), std::bad_cast);
    }

    {
      pib x{ true };
      ASSERT(!x.is_empty() && !x.is<int>() && x.is<bool>());
      bool actual_bool = false;
      x.apply(detect_bool(actual_bool));
      ASSERT_EQ(true, actual_bool);
      ASSERT_EQ(true, x.at<bool>());
      ASSERT_EQ(true, x.get<bool>());
      ASSERT_THROW(x.at<int>(), std::bad_cast);
    }

    {
      pib y{ 42 };
      pib x{ y };
      int actual_int = 0;
      x.apply(detect_int(actual_int));
      ASSERT_EQ(42, actual_int);
    }

    {
      pib y{ 42 };
      pib x;
      x = y;
      int actual_int = 0;
      x.apply(detect_int(actual_int));
      ASSERT_EQ(42, actual_int);
      ASSERT(!y.is_empty() && y.is<int>() && !x.is<bool>());
    }

    {
      pib x;
      x = 42;
      int actual_int = 0;
      x.apply(detect_int(actual_int));
      ASSERT_EQ(42, actual_int);
    }

    {
      pib x{ 3 };
      x = 42;
      int actual_int = 0;
      x.apply(detect_int(actual_int));
      ASSERT_EQ(42, actual_int);
    }

    {
      pib y{ 42 };
      pib x{ std::move(y) };
      int actual_int = 0;
      x.apply(detect_int(actual_int));
      ASSERT_EQ(42, actual_int);
      ASSERT(y.is_empty() && !y.is<int>() && !y.is<bool>());
    }

    {
      pib y{ 42 };
      pib x;
      x = std::move(y);
      int actual_int = 0;
      x.apply(detect_int(actual_int));
      ASSERT_EQ(42, actual_int);
      ASSERT(y.is_empty() && !y.is<int>() && !y.is<bool>());
    }

    {
      pib x;
      x = std::move(42);
      int actual_int = 0;
      x.apply(detect_int(actual_int));
      ASSERT_EQ(42, actual_int);
    }

    {
      pib x{ 3 };
      x = std::move(42);
      int actual_int = 0;
      x.apply(detect_int(actual_int));
      ASSERT_EQ(42, actual_int);
    }

    {
      pib x{ 42 };
      int actual_int = 0;
      ASSERT(!x.is_empty() && x.is<int>() && !x.is<bool>());
      x.apply(detect_int(actual_int));
      ASSERT_EQ(42, actual_int);
      ASSERT_EQ(42, x.at<int>());
      ASSERT_EQ(42, x.get<int>());
      ASSERT_THROW(x.at<bool>(), std::bad_cast);
    }

    {
      pib x{ 42 };
      x.clear();
      ASSERT(x.is_empty() && !x.is<int>() && !x.is<bool>());
    }

    {
      const pib x{ 42 };
      int actual_int = 0;
      ASSERT(!x.is_empty() && x.is<int>() && !x.is<bool>());
      x.apply(detect_const_int(actual_int));
      ASSERT_EQ(42, actual_int);
      ASSERT_EQ(42, x.at<int>());
      ASSERT_EQ(42, x.get<int>());
      ASSERT_THROW(x.at<bool>(), std::bad_cast);
    }
  }

  TEST(variant__with_incomplete_type)
  {
    struct node
    {
      explicit node(int x) : value{ x } {};
      int value;
      ptr<node> next;
    };

    node a{ 1 };
    ASSERT_EQ(1, a.value);
    ASSERT(a.next.is_empty());

    a.next = node(2);
    ASSERT_FALSE(a.next.is_empty());
    ASSERT_EQ(2, a.next.get<node>().value);
    ASSERT(a.next.get<node>().next.is_empty());
  }
} // anonymous namespace
