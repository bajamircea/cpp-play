#pragma once

#include "callback.h"

#include "../cpp_util_lib/intrusive_list.h"

#include <cassert>
#include <utility>

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