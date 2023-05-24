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
  // - pop the smallest node in O(lg(N)) (e.g. when it's duration has passed)
  // - remove a node in O(lg(N)) (e.g. when the timer is cancelled)
  //
  // The node would have intrusive_heap_ptrs as a member.
  // You would store intrusive_heap somewhere (e.g. in the io_service).
  // You the heap access is parametrised with:
  // - your node type
  // - the pointer to the intrusive_heap_ptrs member
  // - the comparison operation

  template<typename Node>
  struct intrusive_heap_ptrs
  {
    Node * left;
    Node * right;
    Node * parent;
  };

  template<typename Node, intrusive_heap_ptrs<Node> Node::*ptrs, typename Compare>
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
      // Assume new node is added
      ++size_;

      // Position of the most significat bit in size_ indicate level in tree,
      // the following bits indicate 0 for left, 1 for right
      std::size_t mask = (size_ > 1) ?
        ((std::size_t)1 << (std::bit_width(size_) - 2)) : (std::size_t)0;

      // Traverse path to new node initial location and insert
      Node * parent = min_node_;
      Node ** link_to_new = &min_node_;
      while (mask != 0)
      {
        parent = *link_to_new;
        auto & parent_ptrs = parent->*ptrs;
        if (size_ & mask)
        {
          link_to_new = &parent_ptrs.right;
        }
        else
        {
          link_to_new = &parent_ptrs.left;
        }
        mask >>= 1;
      }

      auto & new_node_ptrs = new_node->*ptrs;
      new_node_ptrs.left = nullptr;
      new_node_ptrs.right = nullptr;
      new_node_ptrs.parent = parent;
      *link_to_new = new_node;

      Compare compare;
      // Walk up and fix min heap property
      while (new_node_ptrs.parent != nullptr)
      {
        parent = new_node_ptrs.parent;
        if (!compare(*new_node, *parent))
        {
          break;
        }
        node_swap(parent, new_node);
      }
    }

    void remove(Node * node) noexcept
    {
      // Position of the most significat bit in size_ indicate level in tree,
      // the following bits indicate 0 for left, 1 for right
      std::size_t mask = (size_ > 1) ?
        ((std::size_t)1 << (std::bit_width(size_) - 2)) : (std::size_t)0;

      // Traverse path to last node location
      Node ** link_to_last = &min_node_;
      while (mask != 0)
      {
        auto & last_ptrs = (*link_to_last)->*ptrs;
        if (size_ & mask)
        {
          link_to_last = &last_ptrs.right;
        }
        else
        {
          link_to_last = &last_ptrs.left;
        }
        mask >>= 1;
      }

      // Unlink last node
      Node * last = *link_to_last;
      *link_to_last = nullptr;
      --size_;

      if (node == last)
      {
        return;
      }

      auto & last_ptrs = last->*ptrs;

      // Replace node with last
      last_ptrs = node->*ptrs;
      fix_links_from_children(last);
      fix_link_from_parent(last, node);

      Compare compare;
      // Walk down and fix min heap property
      while (true)
      {
        Node * crt_min = last;
        if (last_ptrs.left != nullptr)
        {
          if (compare(*last_ptrs.left, *crt_min))
          {
            crt_min = last_ptrs.left;
          }
        }
        if (last_ptrs.right != nullptr)
        {
          if (compare(*last_ptrs.right, *crt_min))
          {
            crt_min = last_ptrs.right;
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
    void node_swap(Node * parent, Node * child) noexcept
    {
      auto & parent_ptrs = parent->*ptrs;
      auto & child_ptrs = child->*ptrs;

      Node * sibling;
      if (parent_ptrs.left == child)
      {
        sibling = parent_ptrs.right;
        parent_ptrs.left = child_ptrs.left;
        parent_ptrs.right = child_ptrs.right;
        child_ptrs.left = parent;
        child_ptrs.right = sibling;
      }
      else
      {
        sibling = parent_ptrs.left;
        parent_ptrs.left = child_ptrs.left;
        parent_ptrs.right = child_ptrs.right;
        child_ptrs.left = sibling;
        child_ptrs.right = parent;
      }
      child_ptrs.parent = parent_ptrs.parent;
      parent_ptrs.parent = child;

      if (sibling != nullptr)
      {
        (sibling->*ptrs).parent = child;
      }

      fix_links_from_children(parent);
      fix_link_from_parent(child, parent);
    }

    void fix_links_from_children(Node * node) noexcept
    {
      auto & node_ptrs = node->*ptrs;
      if (node_ptrs.left != nullptr)
      {
        (node_ptrs.left->*ptrs).parent = node;
      }
      if (node_ptrs.right != nullptr)
      {
        (node_ptrs.right->*ptrs).parent = node;
      }
    }

    void fix_link_from_parent(Node * new_node, Node * old_node) noexcept
    {
      if ((new_node->*ptrs).parent == nullptr)
      {
        min_node_ = new_node;
      }
      else
      {
        auto & new_parent_ptrs = (new_node->*ptrs).parent->*ptrs;
        if (new_parent_ptrs.left == old_node)
        {
          new_parent_ptrs.left = new_node;
        }
        else
        {
          new_parent_ptrs.right = new_node;
        }
      }
    }
  };
}