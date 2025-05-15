#pragma once

#include <bit>
#include <cassert>
#include <cstddef>

namespace cpp_util
{
  // Intrusive node based min heap
  //
  // The node is a user defined struct which additionally has three pointers
  // (for parent, left and right).
  // A heap is parametrised with:
  // - your node type
  // - the pointers to parent, left, right
  // - the comparison operation
  //
  // e.g. to use for coroutine timers, where each suspended coroutine waiting
  // for the timer to expire declares a node (on the coroutine frame), and all
  // these nodes are chained in such a way that it's possible to perform (in
  // acceptable time complexity and noexcept):
  // - insert a new node in O(lg(N)) (e.g. a new coroutine waiting for a timer)
  // - get the smallest node in O(1) (e.g. the next duration we need to wait for)
  // - pop the smallest node in O(lg(N)) (e.g. after its duration has elapsed)
  // - remove a node in O(lg(N)) (e.g. when the timer is cancelled)
  // You would store the intrusive_heap somewhere (e.g. in the io_service).

  template<typename Node, Node* Node::*parent, Node* Node::*left, Node* Node::*right, typename Compare>
  class intrusive_heap
  {
    Node* min_node_{ nullptr };
    std::size_t size_{ 0 };

  public:
    intrusive_heap() noexcept = default;

    // Not owning, therefore copy does not copy nodes.
    // Delete copy to avoid mistakes where min_node gets updated
    // for a copy out of sync with the original
    intrusive_heap(const intrusive_heap&) = delete;
    intrusive_heap& operator=(const intrusive_heap&) = delete;

    intrusive_heap(intrusive_heap&& other) noexcept :
      min_node_{ other.min_node_ }, size_{ other.size_ }
    {
      other.min_node_ = nullptr;
      other.size_ = 0;
    }

    intrusive_heap& operator=(intrusive_heap&& other) noexcept
    {
      min_node_ = other.min_node_;
      size_ = other.size_;
      other.min_node_ = nullptr;
      other.size_ = 0;
    }

    bool empty() const noexcept
    {
      return (min_node_ == nullptr);
    }

    std::size_t size() const noexcept
    {
      return size_;
    }

    Node* min_node() noexcept
    {
      return min_node_;
    }

    const Node* min_node() const noexcept
    {
      return min_node_;
    }

    void insert(Node* new_node) noexcept
    {
      new_node->*left = nullptr;
      new_node->*right = nullptr;

      if (empty())
      {
        size_ = 1;
        min_node_ = new_node;
        new_node->*parent = nullptr;
        return;
      }

      // Assume new node is added
      ++size_;

      Node* new_node_parent = min_node_;

      // size_ now is at least 2
      // e.g. given if the size_ is now 5 (0...0101 in binary)
      // - the position of most significant 1 bit is
      //   returned by bit_width (3 in the example case)
      //   and indicates the depth/level of the new node
      // - then the following bits indicate the route to the node
      //   with a 0 indicating left edge and a 1 indicating right edge
      // - we use a 1 bit mask against the size to test those bits,
      //   starting with a bit mask 0...0010 in binary for this example.
      //   i.e. a 1 shifted 1 position (i.e. 3 - 2, which we can do as
      //   bit_width is at least 2)
      // - we first stop short of the last bit (mask 0...0001) to reach
      //   the parent (in this case we just do one left), then we use
      //   this last bit to insert to left or right of parent (right in
      //   this case)
      std::size_t mask = ((std::size_t)1 << (std::bit_width(size_) - 2));
      while (mask != 1)
      {
        if (size_ & mask)
        {
          new_node_parent = new_node_parent->*right;
        }
        else
        {
          new_node_parent = new_node_parent->*left;
        }
        mask >>= 1;
      }

      new_node->*parent = new_node_parent;

      if (size_ & mask)
      {
        new_node_parent->*right = new_node;
      }
      else
      {
        new_node_parent->*left = new_node;
      }

      Compare compare;
      // Walk up and fix min heap property
      do
      {
        if (compare(*new_node_parent, *new_node))
        {
          break;
        }
        node_swap(new_node_parent, new_node);
        new_node_parent = new_node->*parent;
      } while(new_node_parent != nullptr);
    }

    void remove(Node* node) noexcept
    {
      assert(size_ > 0);

      if (size_ == 1)
      {
        size_ = 0;
        min_node_ = nullptr;
        return;
      }

      Node* last_node_parent = min_node_;

      // see insert function comment about the mask
      std::size_t mask = ((std::size_t)1 << (std::bit_width(size_) - 2));
      while (mask != 1)
      {
        if (size_ & mask)
        {
          last_node_parent = last_node_parent->*right;
        }
        else
        {
          last_node_parent = last_node_parent->*left;
        }
        mask >>= 1;
      }

      Node* last;

      if (size_ & mask)
      {
        last = last_node_parent->*right;
        last_node_parent->*right = nullptr;
      }
      else
      {
        last = last_node_parent->*left;
        last_node_parent->*left = nullptr;
      }
      --size_;

      if (node == last)
      {
        return;
      }

      // Replace node with last
      last->*parent = node->*parent;
      last->*left = node->*left;
      last->*right = node->*right;
      fix_link_from_parent(last, node);
      fix_links_from_children(last);

      Compare compare;
      // Walk down and fix min heap property
      while (true)
      {
        Node* crt_min = last;
        if (last->*left != nullptr)
        {
          if (compare(*(last->*left), *crt_min))
          {
            crt_min = last->*left;
          }
        }
        if (last->*right != nullptr)
        {
          if (compare(*(last->*right), *crt_min))
          {
            crt_min = last->*right;
          }
        }
        if (crt_min == last)
        {
          break;
        }
        node_swap(last, crt_min);
      }
    }

    void pop_min() noexcept
    {
      remove(min_node_);
    }

  private:
    void node_swap(Node* old_parent, Node* new_parent) noexcept
    {
      Node* sibling;
      if (old_parent->*left == new_parent)
      {
        sibling = old_parent->*right;
        old_parent->*left = new_parent->*left;
        old_parent->*right = new_parent->*right;
        new_parent->*left = old_parent;
        new_parent->*right = sibling;
      }
      else
      {
        sibling = old_parent->*left;
        old_parent->*left = new_parent->*left;
        old_parent->*right = new_parent->*right;
        new_parent->*left = sibling;
        new_parent->*right = old_parent;
      }
      new_parent->*parent = old_parent->*parent;
      old_parent->*parent = new_parent;

      if (sibling != nullptr)
      {
        sibling->*parent = new_parent;
      }

      fix_links_from_children(old_parent);
      fix_link_from_parent(new_parent, old_parent);
    }

    void fix_links_from_children(Node* node) noexcept
    {
      if (node->*left != nullptr)
      {
        node->*left->*parent = node;
      }
      if (node->*right != nullptr)
      {
        node->*right->*parent = node;
      }
    }

    void fix_link_from_parent(Node* new_node, Node* old_node) noexcept
    {
      Node* parent_parent = new_node->*parent;
      if (parent_parent == nullptr)
      {
        min_node_ = new_node;
      }
      else if (parent_parent->left == old_node)
      {
        parent_parent->left = new_node;
      }
      else
      {
        parent_parent->right = new_node;
      }
    }
  };
}