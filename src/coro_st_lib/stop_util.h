#pragma once

#include "callback.h"

#include <cassert>
#include <cstddef>
#include <utility>

#include <stop_token>

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
  // - no cached size
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

namespace coro_st
{
  struct stop_list_node
  {
    stop_list_node* next{ nullptr };
    stop_list_node* prev{ nullptr };
    callback cb{};

    stop_list_node() noexcept = default;

    stop_list_node(const stop_list_node&) = delete;
    stop_list_node& operator=(const stop_list_node&) = delete;
  };

  using stop_list = cpp_util::intrusive_list<stop_list_node, &stop_list_node::next, &stop_list_node::prev>;

  class stop_source;

  template <typename Fn>
  class stop_callback;

  class stop_token
  {
    friend stop_source;

    template <typename Fn>
    friend class stop_callback;

    stop_source* source_ = nullptr;

    explicit stop_token(stop_source* source) noexcept : source_{ source }
    {
    }

  public:
    stop_token(const stop_token&) noexcept = default;
    stop_token& operator=(const stop_token&) noexcept = default;

    bool stop_requested() const noexcept;
  };

  class stop_source
  {
    template <typename Fn>
    friend class stop_callback;

    bool stop_{ false };
    stop_list callbacks_{};
  public:
    stop_source() noexcept = default;

    stop_source(const stop_source&) = delete;
    stop_source& operator=(const stop_source&) = delete;

    bool stop_requested() const noexcept
    {
      return stop_;
    }

    bool request_stop() noexcept
    {
      if (stop_)
      {
        return false;
      }
      stop_ = true;
      while(true)
      {
        stop_list_node* node = callbacks_.pop_front();
        if (node == nullptr)
        {
          break;
        }
        callback copy_cb = node->cb;
        node->cb = callback{};
        copy_cb.invoke();
      }
      return true;
    }

    stop_token get_token() noexcept
    {
      return stop_token{ this };
    }
  };

  inline bool stop_token::stop_requested() const noexcept
  {
    assert(source_ != nullptr);
    return source_->stop_requested();
  }

  template<>
  class stop_callback<callback>
  {
    stop_source* source_ = nullptr;
    stop_list_node node_;
  public:
    stop_callback(stop_token token, callback fn) noexcept :
      source_{ token.source_ }
    {
      assert(source_ != nullptr);
      assert(fn.is_callable());
      if (source_->stop_requested())
      {
        fn();
      }
      else
      {
        node_.cb = fn;
        source_->callbacks_.push_back(&node_);
      }
    }

    stop_callback(const stop_callback&) = delete;
    stop_callback& operator=(const stop_callback&) = delete;

    ~stop_callback()
    {
      if (node_.cb.is_callable())
      {
        source_->callbacks_.remove(&node_);
      }
    }
  };

  template <typename Fn>
  class stop_callback
  {
    stop_source* source_ = nullptr;
    Fn fn_;
    stop_list_node node_;
  public:
    stop_callback(stop_token token, Fn&& fn) noexcept :
      source_{ token.source_ }, fn_{ std::move(fn) }
    {
      assert(source_ != nullptr);
      if (source_->stop_requested())
      {
        fn_();
      }
      else
      {
        node_.cb = make_member_callback<&stop_callback::invoke>(this);
        source_->callbacks_.push_back(&node_);
      }
    }

    stop_callback(const stop_callback&) = delete;
    stop_callback& operator=(const stop_callback&) = delete;

    ~stop_callback()
    {
      if (node_.cb.is_callable())
      {
        source_->callbacks_.remove(&node_);
      }
    }

  private:
    void invoke() noexcept
    {
      fn_();
    }
  };
}