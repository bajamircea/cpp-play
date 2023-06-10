#pragma once

#include <bit>
#include <cassert>
#include <cstddef>

namespace cpp_util
{
  // Intrusive node based min heap
  //
  // e.g. to use for coroutine timers, where each suspended coroutine waiting
  // for the timer to expire declares a node (on the coroutine frame), and all
  // theese nodes are chained in such a way that it's possible to perform (in
  // acceptable time complexity and noexcept):
  // - insert a new node in O(lg(N)) (e.g. a new coroutine waiting for a timer)
  // - get the smallest node in O(1) (e.g. the next duration we need to wait for)
  // - pop the smallest node in O(lg(N)) (e.g. after its duration has elapsed)
  // - remove a node in O(lg(N)) (e.g. when the timer is cancelled)
  //
  // The node would have intrusive_heap_ptrs as a member.
  // You would store intrusive_heap somewhere (e.g. in the io_service).
  // You the heap access is parametrised with:
  // - your node type
  // - the pointers to parent, left, right
  // - the comparison operation

  template<typename Node, Node * Node::*parent, Node * Node::*left, Node * Node::*right, typename Compare>
  class intrusive_heap
  {
    Node * min_node_ = nullptr;
    std::size_t size_ = 0;

  public:
    bool empty() const noexcept
    {
      return (min_node_ == nullptr);
    }

    std::size_t size() const noexcept
    {
      return size_;
    }

    Node * min_node() noexcept
    {
      return min_node_;
    }

    const Node * min_node() const noexcept
    {
      return min_node_;
    }

    void insert(Node * new_node) noexcept
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

      Node * new_node_parent = min_node_;

      // e.g.
      // if the size_ is now 5 (0110)
      // - bit_width is 3
      // - mask needs to be for the bit at position 2 (i.e. 3 - 1)
      //   size_ now is at least 2, bit_width also is at least 2,
      //   so it's fine to decrement
      // - we go right to find the new parent
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

    void remove(Node * node) noexcept
    {
      assert(size_ > 0);

      if (size_ == 1)
      {
        size_ = 0;
        min_node_ = nullptr;
        return;
      }

      Node * last_node_parent = min_node_;

      // e.g.
      // if the size_ is now 5 (0110)
      // - bit_width is 3
      // - mask needs to be for the bit at position 2 (i.e. 3 - 1)
      //   size_ now is at least 2, bit_width also is at least 2,
      //   so it's fine to decrement
      // - we go right to find the new parent
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

      Node * last;

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
        Node * crt_min = last;
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
    void node_swap(Node * old_parent, Node * new_parent) noexcept
    {
      Node * sibling;
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

    void fix_links_from_children(Node * node) noexcept
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

    void fix_link_from_parent(Node * new_node, Node * old_node) noexcept
    {
      Node * parent_parent = new_node->*parent;
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