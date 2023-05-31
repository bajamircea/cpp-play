#pragma once

#include <cstddef>

namespace cpp_util
{
  // Intrusive list container
  //
  // Note: useful to use by deleting a node based on a reference to that node
  // - intrusive
  // - non-ownning
  // - double linked
  // - linear
  // - end iterator is nullptr
  // - header points to head and tail
  // - no dummy node
  // - cached size
  template<typename Node>
  struct intrusive_list_ptrs
  {
    Node * next {nullptr};
    Node * prev {nullptr};
  };

  template<typename Node, intrusive_list_ptrs<Node> Node::*ptrs>
  class intrusive_list
  {
    Node * head_;
    Node * tail_;
    std::size_t size_;
  public:
    class const_iterator;

    class iterator
    {
      Node * crt_; // nullptr for the end
    public:
      iterator() noexcept : crt_{ nullptr }
      {
      }

      explicit iterator(Node * crt) noexcept : crt_{ crt }
      {
      }

      Node & operator*()
      {
        return *crt_;
      }

      iterator & operator++()
      {
        crt_ = (crt_->*ptrs).next;
        return *this;
      }

      friend bool operator==(const iterator & x, const iterator & y)
      {
        return x.crt_ == y.crt_;
      }

      friend bool operator!=(const iterator & x, const iterator & y)
      {
        return !(x == y);
      }

      friend class list;
      friend class const_iterator;
    };

    class const_iterator
    {
      const Node * crt_; // nullptr for the end
    public:
      const_iterator() noexcept : crt_{ nullptr }
      {
      }

      explicit const_iterator(const Node * crt) noexcept : crt_{ crt }
      {
      }

      const Node & operator*()
      {
        return *crt_;
      }

      const_iterator & operator++()
      {
        crt_ = crt_->*ptrs.next;
        return *this;
      }

      friend bool operator==(const const_iterator & x, const const_iterator & y)
      {
        return x.crt_ == y.crt_;
      }

      friend bool operator!=(const const_iterator & x, const const_iterator & y)
      {
        return !(x == y);
      }

      // mix and match comparisons
      friend bool operator==(const iterator & x, const const_iterator & y)
      {
        return x.crt_ == y.crt_;
      }

      friend bool operator!=(const iterator & x, const const_iterator & y)
      {
        return !(x == y);
      }

      friend bool operator==(const const_iterator & x, const iterator & y)
      {
        return x.crt_ == y.crt_;
      }

      friend bool operator!=(const const_iterator & x, const iterator & y)
      {
        return !(x == y);
      }
    };

    intrusive_list() noexcept : head_{ nullptr }, tail_{ nullptr }, size_{ 0 }
    {
    }

    // Not owning, therefore copy does not copy nodes.
    // Delete copy to avoid mistakes where head and tail get updated
    // for a copy out of sync with the original
    intrusive_list(const intrusive_list &) = delete;
    intrusive_list & operator=(const intrusive_list &) = delete;

    intrusive_list(intrusive_list && other) noexcept : head_{ other.head_ }, tail_{ other.tail_ }, size_{ other.size_ }
    {
      other.head_ = nullptr;
      other.tail_ = nullptr;
      other.size_ = 0;
    }

    intrusive_list & operator=(intrusive_list && other) noexcept
    {
      head_ = other.head_;
      tail_ = other.tail_;
      size_ = other.size_;
      other.head_ = nullptr;
      other.tail_ = nullptr;
      other.size_ = 0;
    }

    bool empty() const noexcept
    {
      return head_ == nullptr;
    }

    size_t size() const noexcept
    {
      return size_;
    }

    iterator begin() noexcept
    {
      return iterator(head_);
    }

    const_iterator begin() const noexcept
    {
      return const_iterator(head_);
    }

    const_iterator cbegin() const noexcept
    {
      return const_iterator(head_);
    }

    iterator end() noexcept
    {
      return iterator();
    }

    const_iterator end() const noexcept
    {
      return const_iterator();
    }

    const_iterator cend() const noexcept
    {
      return const_iterator();
    }

    void push_back(Node * what) noexcept
    {
      (what->*ptrs).next = nullptr; // linear list, end is nullptr
      (what->*ptrs).prev = tail_; // points to tail (could be nullptr)
      if (nullptr == tail_)
      {
        head_ = what;
        tail_ = what;
      }
      else
      {
        (tail_->*ptrs).next = what;
        tail_ = what;
      }
      ++size_;
    }

    void remove(Node * what) noexcept
    {
      if (what == head_)
      {
        head_ = (what->*ptrs).next;
      }
      else
      {
        ((what->*ptrs).prev->*ptrs).next = (what->*ptrs).next;
      }
      if (what == tail_)
      {
        tail_ = (what->*ptrs).prev;
      }
      else
      {
        ((what->*ptrs).next->*ptrs).prev = (what->*ptrs).prev;
      }
      (what->*ptrs).next = nullptr;
      (what->*ptrs).prev = nullptr;
      --size_;
    }
  };
};