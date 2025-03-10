#pragma once

#include "../cpp_util_lib/intrusive_list.h"

#include <cassert>
#include <utility>

namespace coro::st
{
  struct stop_list_node
  {
    stop_list_node * next{ nullptr };
    stop_list_node * prev{ nullptr };
    void (*fn)(void* x) noexcept { nullptr };
    void* x{ nullptr };
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

    stop_token(stop_source* source) : source_{ source }
    {
    }
  public:
    stop_token(const stop_token&) = default;
    stop_token& operator=(const stop_token&) = default;

    bool stop_requested() const noexcept;
  };

  class stop_source
  {
    friend stop_token;

    template <typename Fn>
    friend class stop_callback;

    bool stop_{ false };
    stop_list callbacks_{};
  public:
    stop_source()
    {
    }
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
      for(stop_list_node* node = callbacks_.front(); node != nullptr; node = node->next)
      {
        node->fn(node->x);
      }
      return true;
    }

    stop_token get_token() noexcept
    {
      return { this };
    }
  };

  inline bool stop_token::stop_requested() const noexcept
  {
    assert(source_ != nullptr);
    return source_->stop_requested();
  }

  template <typename Fn>
  class stop_callback
  {
    stop_source* source_ = nullptr;
    Fn fn_;
    stop_list_node node_;
  public:
    stop_callback(stop_token token, Fn && fn) : source_{ token.source_ }, fn_{ std::move(fn) }
    {
      assert(source_ != nullptr);
      if (source_->stop_requested())
      {
        fn_();
      }
      else
      {
        node_.fn = invoke;
        node_.x = this;
        source_->callbacks_.push_back(&node_);
      }
    }

    stop_callback(const stop_callback&) = delete;
    stop_callback& operator=(const stop_callback&) = delete;

    ~stop_callback()
    {
      if (node_.x != nullptr)
      {
        source_->callbacks_.remove(&node_);
      }
    }
  private:
    static void invoke(void* x) noexcept
    {
      assert(x != nullptr);
      stop_callback* self = reinterpret_cast<stop_callback*>(x);
      self->fn_();
    }
  };
}