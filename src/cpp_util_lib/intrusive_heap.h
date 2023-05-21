#pragma once

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
  // You would store intrusive_heap_object somewhere (e.g. in the io_service).
  // You would manipulate the heap via intrusive_heap instantiated with:
  // - your node type
  // - the pointer to the intrusive_heap_ptrs member
  // - the comparison operation

  struct intrusive_heap_ptrs
  {
    void * left;
    void * right;
    void * parent;
  };

  struct intrusive_heap_object
  {
    void * min_node = nullptr;
    std::size_t size = 0;
  };

  template<typename Node, intrusive_heap_ptrs Node::*ptrs, typename Compare>
  struct intrusive_heap
  {
    static bool empty(const intrusive_heap_object& heap) noexcept
    {
      return heap.min_node == nullptr;
    }

    static Node * min_node(intrusive_heap_object& heap) noexcept
    {
      return ptr_to_node(heap.min_node);
    }

    static void insert(intrusive_heap_object& heap, Node * new_node) noexcept
    {
      // Assume new node is added
      ++heap.size;

      // Calculate path bits: 0 (even) means left, 1 (odd) means right
      std::size_t path = 0;
      std::size_t steps = 0;
      for (std::size_t direction = heap.size; direction >= 2; direction /= 2)
      {
        path <<= 1;
        path |= (direction & 1);
        ++steps;
      }

      // Traverse path to new node initial location and insert
      Node * parent = min_node(heap);
      Node ** link_to_new = reinterpret_cast<Node **>(&heap.min_node);
      while (steps > 0)
      {
        parent = *link_to_new;
        intrusive_heap_ptrs & parent_ptrs = parent->*ptrs;
        if (path & 1)
        {
          link_to_new = reinterpret_cast<Node **>(&parent_ptrs.right);
        }
        else
        {
          link_to_new = reinterpret_cast<Node **>(&parent_ptrs.left);
        }
        path >>= 1;
        --steps;
      }

      intrusive_heap_ptrs & new_node_ptrs = new_node->*ptrs;
      new_node_ptrs.left = nullptr;
      new_node_ptrs.right = nullptr;
      new_node_ptrs.parent = parent;
      *link_to_new = new_node;

      Compare compare;
      // Walk up and fix min heap property
      while (new_node_ptrs.parent != nullptr)
      {
        parent = ptr_to_node(new_node_ptrs.parent);
        if (!compare(*new_node, *parent))
        {
          break;
        }
        node_swap(heap, parent, new_node);
      }
    }

    static void remove(intrusive_heap_object& heap, Node * node) noexcept
    {
      // Calculate path bits: 0 (even) means left, 1 (odd) means right
      std::size_t path = 0;
      std::size_t steps = 0;
      for (std::size_t direction = heap.size; direction >= 2; direction /= 2)
      {
        path <<= 1;
        path |= (direction & 1);
        ++steps;
      }

      // Traverse path to last node location
      Node ** link_to_last = reinterpret_cast<Node **>(&heap.min_node);
      while (steps > 0)
      {
        intrusive_heap_ptrs & last_ptrs = (*link_to_last)->*ptrs;
        if (path & 1)
        {
          link_to_last = reinterpret_cast<Node **>(&last_ptrs.right);
        }
        else
        {
          link_to_last = reinterpret_cast<Node **>(&last_ptrs.left);
        }
        path >>= 1;
        --steps;
      }

      // Unlink last node
      Node * last = *link_to_last;
      *link_to_last = nullptr;
      --heap.size;

      if (node == last)
      {
        return;
      }

      intrusive_heap_ptrs & last_ptrs = last->*ptrs;
      intrusive_heap_ptrs & node_ptrs = node->*ptrs;

      // Replace node with last
      last_ptrs.left = node_ptrs.left;
      last_ptrs.right = node_ptrs.right;
      last_ptrs.parent = node_ptrs.parent;

      if (last_ptrs.left != nullptr)
      {
        (ptr_to_node(last_ptrs.left)->*ptrs).parent = last;
      }
      if (last_ptrs.right != nullptr)
      {
        (ptr_to_node(last_ptrs.right)->*ptrs).parent = last;
      }

      if (last_ptrs.parent == nullptr)
      {
        heap.min_node = last;
      }
      else
      {
        intrusive_heap_ptrs & new_parent_ptrs = ptr_to_node(last_ptrs.parent)->*ptrs;
        if (new_parent_ptrs.left == node)
        {
          new_parent_ptrs.left = last;
        }
        else
        {
          new_parent_ptrs.right = last;
        }
      }

      Compare compare;
      // Walk down and fix min heap property
      while (true)
      {
        Node * min_node = last;
        if (last_ptrs.left != nullptr)
        {
          Node * left = ptr_to_node(last_ptrs.left);
          if (compare(*left, *min_node))
          {
            min_node = left;
          }
        }
        if (last_ptrs.right != nullptr)
        {
          Node * right = ptr_to_node(last_ptrs.right);
          if (compare(*right, *min_node))
          {
            min_node = right;
          }
        }
        if (min_node == last)
        {
          break;
        }
        node_swap(heap, last, min_node);
      }
    }

    static void pop_min(intrusive_heap_object& heap) noexcept
    {
      remove(heap, ptr_to_node(heap.min_node));
    }

  private:
    static Node * ptr_to_node(void * p) noexcept
    {
      return reinterpret_cast<Node *>(p);
    }

    static void node_swap(intrusive_heap_object& heap, Node * parent, Node * child) noexcept
    {
      intrusive_heap_ptrs & parent_ptrs = parent->*ptrs;
      intrusive_heap_ptrs & child_ptrs = child->*ptrs;

      Node * sibling;
      if (parent_ptrs.left == child)
      {
        sibling = ptr_to_node(parent_ptrs.right);
        parent_ptrs.left = child_ptrs.left;
        parent_ptrs.right = child_ptrs.right;
        child_ptrs.left = parent;
        child_ptrs.right = sibling;
      }
      else
      {
        sibling = ptr_to_node(parent_ptrs.left);
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

      if (parent_ptrs.left != nullptr)
      {
        (ptr_to_node(parent_ptrs.left)->*ptrs).parent = parent;
      }
      if (parent_ptrs.right != nullptr)
      {
        (ptr_to_node(parent_ptrs.right)->*ptrs).parent = parent;
      }

      if (child_ptrs.parent == nullptr)
      {
        heap.min_node = child;
      }
      else
      {
        intrusive_heap_ptrs & new_parent_ptrs = ptr_to_node(child_ptrs.parent)->*ptrs;
        if (new_parent_ptrs.left == parent)
        {
          new_parent_ptrs.left = child;
        }
        else
        {
          new_parent_ptrs.right = child;
        }
      }
    }
  };
}