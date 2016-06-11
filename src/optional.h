#pragma once

#include <utility>

namespace optional
{
  template<typename T>
  class ptr
  {
  public:
    ptr() noexcept :
      p_{ nullptr }
    {
    }

    ~ptr()
    {
      delete p_;
    }

    ptr(const ptr & other)
    {
      if (nullptr == other.p_)
      {
        p_ = nullptr;
      }
      else
      {
        p_ = new T(*other.p_);
      }
    }

    ptr & operator=(const ptr & other)
    {
      if (this != &other)
      {
        delete p_;
        p_ = nullptr;

        p_ = new T(*other.p_);
      }
      return *this;
    }

    ptr(ptr && other) noexcept :
      p_{ other.p_ }
    {
      other.p_ = nullptr;
    }

    ptr & operator=(ptr && other) noexcept
    {
      T * tmp = other.p_;
      other.p_ = nullptr;
      delete p_;
      p_ = tmp;
      return *this;
    }

    explicit ptr(const T & other) :
      p_{ new T(other) }
    {
    }

    ptr & operator=(const T & other)
    {
      if (p_ != &other)
      {
        delete p_;
        p_ = nullptr;

        p_ = new T(other);
      }
      return *this;
    }

    explicit ptr(const T && other) :
      p_{ new T( std::move(other) ) }
    {
    }

    ptr & operator=(T && other)
    {
      if (p_ != &other)
      {
        delete p_;
        p_ = nullptr;

        p_ = new T(std::move(other));
      }
      return *this;
    }

    bool is_empty()
    {
      return nullptr == p_;
    }

    T & get()
    {
      return *p_;
    }

  private:
    T * p_;
  };
} // namespace optional
