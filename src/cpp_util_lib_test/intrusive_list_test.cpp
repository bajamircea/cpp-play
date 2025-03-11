#include "../test_lib/test.h"

#include "../cpp_util_lib/intrusive_list.h"

namespace
{
  struct list_node
  {
    list_node* next{};
    list_node* prev{};
    int value{};

    list_node() noexcept = default;

    list_node(const list_node&) = delete;
    list_node& operator=(const list_node&) = delete;
  };

  using list = cpp_util::intrusive_list<list_node, &list_node::next, &list_node::prev>;

  TEST(intrusive_list_simple)
  {
    list x;

    ASSERT_TRUE(x.empty());

    list_node e0;
    e0.value = 40;
    x.push_back(&e0);
    ASSERT_FALSE(x.empty());
    ASSERT_EQ(&e0, x.front());
    ASSERT_EQ(&e0, x.back());
    ASSERT_EQ(nullptr, e0.next);
    ASSERT_EQ(nullptr, e0.prev);

    list_node e1;
    e1.value = 41;
    x.push_back(&e1);
    ASSERT_FALSE(x.empty());
    ASSERT_EQ(&e0, x.front());
    ASSERT_EQ(&e1, x.back());
    ASSERT_EQ(&e1, e0.next);
    ASSERT_EQ(nullptr, e0.prev);
    ASSERT_EQ(nullptr, e1.next);
    ASSERT_EQ(&e0, e1.prev);

    list_node e2;
    e2.value = 42;
    x.push_back(&e2);
    ASSERT_FALSE(x.empty());
    ASSERT_EQ(&e0, x.front());
    ASSERT_EQ(&e2, x.back());
    ASSERT_EQ(&e1, e0.next);
    ASSERT_EQ(nullptr, e0.prev);
    ASSERT_EQ(&e2, e1.next);
    ASSERT_EQ(&e0, e1.prev);
    ASSERT_EQ(nullptr, e2.next);
    ASSERT_EQ(&e1, e2.prev);

    x.remove(&e1);
    ASSERT_FALSE(x.empty());
    ASSERT_EQ(&e0, x.front());
    ASSERT_EQ(&e2, x.back());
    ASSERT_EQ(&e2, e0.next);
    ASSERT_EQ(nullptr, e0.prev);
    ASSERT_EQ(nullptr, e2.next);
    ASSERT_EQ(&e0, e2.prev);

    x.remove(&e0);
    ASSERT_FALSE(x.empty());
    ASSERT_EQ(&e2, x.front());
    ASSERT_EQ(&e2, x.back());
    ASSERT_EQ(nullptr, e2.next);
    ASSERT_EQ(nullptr, e2.prev);

    x.remove(&e2);
    ASSERT_TRUE(x.empty());
    ASSERT_EQ(nullptr, x.front());
    ASSERT_EQ(nullptr, x.back());
  }
}