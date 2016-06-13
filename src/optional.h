#pragma once

#include <utility>
#include <typeinfo>

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
        if (nullptr == p_)
        {
          p_ = new T(*other.p_);
        }
        else
        {
          *p_ = *other.p_;
        }
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
        if (nullptr == p_)
        {
          p_ = new T(other);
        }
        else
        {
          *p_ = other;
        }
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
        if (nullptr == p_)
        {
          p_ = new T(std::move(other));
        }
        else
        {
          *p_ = std::move(other);
        }
      }
      return *this;
    }

    bool is_empty() const noexcept
    {
      return nullptr == p_;
    }

    T & get()
    {
      return *p_;
    }

    const T & get() const
    {
      return *p_;
    }

    T & at()
    {
      if (is_empty())
      {
        throw std::bad_cast();
      }
      return get();
    }

    const T & at() const
    {
      if (is_empty())
      {
        throw std::bad_cast();
      }
      return get();
    }

    void clear() noexcept
    {
      delete p_;
      p_ = nullptr;
    }

  private:
    T * p_;
  };
} // namespace optional
