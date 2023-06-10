#pragma once

#include <cstddef>

namespace cpp_util
{
  // Intrusive queue list
  template<typename Node, Node * Node::*next>
  class intrusive_queue
  {
    Node * head_;
    Node * tail_;
  public:
    intrusive_queue() noexcept : head_{ nullptr }, tail_{ nullptr }
    {
    }

    // Not owning, therefore copy does not copy nodes.
    // Delete copy to avoid mistakes where head and tail get updated
    // for a copy out of sync with the original
    intrusive_queue(const intrusive_queue &) = delete;
    intrusive_queue & operator=(const intrusive_queue &) = delete;

    intrusive_queue(intrusive_queue && other) noexcept : head_{ other.head_ }, tail_{ other.tail_ }
    {
      other.head_ = nullptr;
      other.tail_ = nullptr;
    }

    intrusive_queue & operator=(intrusive_queue && other) noexcept
    {
      head_ = other.head_;
      tail_ = other.tail_;
      other.head_ = nullptr;
      other.tail_ = nullptr;
    }

    bool empty() const noexcept
    {
      return head_ == nullptr;
    }

    void push(Node * what) noexcept
    {
      what->*next = nullptr; // linear list, end is nullptr
      if (nullptr == tail_)
      {
        head_ = what;
      }
      else
      {
        tail_->*next = what;
      }
      tail_ = what;
    }

    Node * pop() noexcept
    {
      if (head_ == nullptr)
      {
        return nullptr;
      }
      Node * return_value = head_;
      head_ = head_->*next;
      if (head_ == nullptr)
      {
        tail_ = nullptr;
      }
      return return_value;
    }
  };
}