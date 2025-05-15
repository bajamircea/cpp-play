#include "../test_lib/test.h"

#include "../cpp_util_lib/intrusive_heap.h"

#include <string>

namespace
{
  struct heap_node
  {
    heap_node* parent;
    heap_node* left;
    heap_node* right;
    std::string key;
  };

  struct heap_node_compare
  {
    bool operator()(const heap_node& x, const heap_node& y)
    {
      return x.key < y.key;
    }
  };

  using heap = cpp_util::intrusive_heap<heap_node, &heap_node::parent, &heap_node::left, &heap_node::right, heap_node_compare>;

  TEST(intrusive_heap_simple)
  {
    heap x;

    ASSERT_TRUE(x.empty());
    ASSERT_EQ(0, x.size());
    ASSERT_EQ(nullptr, x.min_node());

    heap_node foo;
    foo.key = "foo";
    heap_node bar;
    bar.key = "bar";
    heap_node buzz;
    buzz.key = "buzz";
    heap_node wozz;
    wozz.key = "wozz";

    x.insert(&foo);

    ASSERT_FALSE(x.empty());
    ASSERT_EQ(1, x.size());
    ASSERT_EQ(&foo, x.min_node());
    ASSERT_EQ(nullptr, foo.parent);
    ASSERT_EQ(nullptr, foo.left);
    ASSERT_EQ(nullptr, foo.right);

    x.insert(&bar);

    ASSERT_FALSE(x.empty());
    ASSERT_EQ(2, x.size());
    ASSERT_EQ(&bar, x.min_node());
    ASSERT_EQ(&bar, foo.parent);
    ASSERT_EQ(nullptr, foo.left);
    ASSERT_EQ(nullptr, foo.right);
    ASSERT_EQ(nullptr, bar.parent);
    ASSERT_EQ(&foo, bar.left);
    ASSERT_EQ(nullptr, bar.right);

    x.insert(&buzz);

    ASSERT_FALSE(x.empty());
    ASSERT_EQ(3, x.size());
    ASSERT_EQ(&bar, x.min_node());
    ASSERT_EQ(&bar, foo.parent);
    ASSERT_EQ(nullptr, foo.left);
    ASSERT_EQ(nullptr, foo.right);
    ASSERT_EQ(nullptr, bar.parent);
    ASSERT_EQ(&foo, bar.left);
    ASSERT_EQ(&buzz, bar.right);
    ASSERT_EQ(&bar, buzz.parent);
    ASSERT_EQ(nullptr, buzz.left);
    ASSERT_EQ(nullptr, buzz.right);

    x.insert(&wozz);

    ASSERT_FALSE(x.empty());
    ASSERT_EQ(4, x.size());
    ASSERT_EQ(&bar, x.min_node());
    ASSERT_EQ(&bar, foo.parent);
    ASSERT_EQ(&wozz, foo.left);
    ASSERT_EQ(nullptr, foo.right);
    ASSERT_EQ(nullptr, bar.parent);
    ASSERT_EQ(&foo, bar.left);
    ASSERT_EQ(&buzz, bar.right);
    ASSERT_EQ(&bar, buzz.parent);
    ASSERT_EQ(nullptr, buzz.left);
    ASSERT_EQ(nullptr, buzz.right);
    ASSERT_EQ(&foo, wozz.parent);
    ASSERT_EQ(nullptr, wozz.left);
    ASSERT_EQ(nullptr, wozz.right);

    x.pop_min();

    ASSERT_FALSE(x.empty());
    ASSERT_EQ(3, x.size());
    ASSERT_EQ(&buzz, x.min_node());
    ASSERT_EQ(&buzz, foo.parent);
    ASSERT_EQ(nullptr, foo.left);
    ASSERT_EQ(nullptr, foo.right);
    ASSERT_EQ(nullptr, buzz.parent);
    ASSERT_EQ(&foo, buzz.left);
    ASSERT_EQ(&wozz, buzz.right);
    ASSERT_EQ(&buzz, wozz.parent);
    ASSERT_EQ(nullptr, wozz.left);
    ASSERT_EQ(nullptr, wozz.right);

    x.pop_min();

    ASSERT_FALSE(x.empty());
    ASSERT_EQ(2, x.size());
    ASSERT_EQ(&foo, x.min_node());
    ASSERT_EQ(nullptr, foo.parent);
    ASSERT_EQ(&wozz, foo.left);
    ASSERT_EQ(nullptr, foo.right);
    ASSERT_EQ(&foo, wozz.parent);
    ASSERT_EQ(nullptr, wozz.left);
    ASSERT_EQ(nullptr, wozz.right);

    x.pop_min();

    ASSERT_FALSE(x.empty());
    ASSERT_EQ(1, x.size());
    ASSERT_EQ(&wozz, x.min_node());
    ASSERT_EQ(nullptr, wozz.parent);
    ASSERT_EQ(nullptr, wozz.left);
    ASSERT_EQ(nullptr, wozz.right);

    x.pop_min();

    ASSERT_TRUE(x.empty());
    ASSERT_EQ(0, x.size());
    ASSERT_EQ(nullptr, x.min_node());
  }
} // anonymous namespace