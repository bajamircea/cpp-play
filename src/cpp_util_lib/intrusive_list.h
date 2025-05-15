#pragma once

#include <cstddef>

namespace cpp_util
{
  // Intrusive list
  //
  // Note: useful to use by deleting a node based on a reference to that node
  // - intrusive
  // - non-ownning
  // - double linked
  // - linear
  // - end iterator is nullptr
  // - header points to head and tail
  // - no dummy node
  template<typename Node, Node* Node::*next, Node* Node::*prev>
  class intrusive_list
  {
    Node* head_{ nullptr };
    Node* tail_{ nullptr };
  public:
    intrusive_list() noexcept = default;

    // Not owning, therefore copy does not copy nodes.
    // Delete copy to avoid mistakes where head and tail get updated
    // for a copy out of sync with the original
    intrusive_list(const intrusive_list&) = delete;
    intrusive_list& operator=(const intrusive_list&) = delete;

    intrusive_list(intrusive_list&& other) noexcept :
      head_{ other.head_ }, tail_{ other.tail_ }
    {
      other.head_ = nullptr;
      other.tail_ = nullptr;
    }

    intrusive_list& operator=(intrusive_list&& other) noexcept
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

    void push_back(Node* what) noexcept
    {
      what->*next = nullptr;
      what->*prev = tail_;
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

    void remove(Node* what) noexcept
    {
      if (what == head_)
      {
        head_ = what->*next;
      }
      else
      {
        what->*prev->*next = what->*next;
      }
      if (what == tail_)
      {
        tail_ = what->*prev;
      }
      else
      {
        what->*next->*prev = what->*prev;
      }
    }

    Node* front() noexcept
    {
      return head_;
    }

    Node* back() noexcept
    {
      return tail_;
    }

    Node* pop_front() noexcept
    {
      if (head_ == nullptr)
      {
        return nullptr;
      }
      Node* what = head_;
      head_ = what->*next;
      if (what == tail_)
      {
        tail_ = nullptr;
      }
      else
      {
        what->*next->*prev = nullptr;
      }
      return what;
    }
  };
}