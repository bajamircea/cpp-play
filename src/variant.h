#pragma once

#include <utility>
#include <type_traits>
#include <typeinfo>

namespace variant
{
  namespace impl
  {
    template<typename T, typename ... Ts>
    struct index_of_type;

    template<typename T, typename ... Ts>
    struct index_of_type<T, T, Ts ...> :
      std::integral_constant<std::size_t, 0>
    {
    };

    template<typename T, typename First, typename ... Ts>
    struct index_of_type<T, First, Ts ...> :
      std::integral_constant<std::size_t, 1 + index_of_type<T, Ts ...>::value>
    {
    };

    template<typename T, typename ... Ts>
    struct is_variant_contents :
      std::false_type
    {
    };

    template<typename T, typename ... Ts>
    struct is_variant_contents<T, T, Ts ...> :
      std::true_type
    {
    };

    template<typename T, typename First, typename ... Ts>
    struct is_variant_contents<T, First, Ts ...> :
      is_variant_contents<T, Ts ...>
    {
    };

    struct variant_ptr
    {
      std::size_t idx;
      void * p;
    };

    template<typename F, typename ... Ts>
    struct apply
    {
      static inline auto call(std::size_t, const variant_ptr &, F & f)
      {
        return f();
      }
    };

    template<typename F, typename First, typename ... Ts>
    struct apply<F, First, Ts ...>
    {
      static inline auto call(std::size_t idx, const variant_ptr & v, F & f)
      {
        if (v.idx == idx)
        {
          return f(*static_cast<First *>(v.p));
        }
        else
        {
          return apply<F, Ts ... >::call(idx + 1, v, f);
        }
      }
    };

    struct ref_deleter
    {
      template<typename T>
      void operator()(T & t) noexcept
      {
        delete &t;
      }

      void operator()() noexcept
      {
      }
    };

    struct copier
    {
      template<typename T>
      void * operator()(const T & t) noexcept
      {
        return new T(t);
      }

      void * operator()() noexcept
      {
        return nullptr;
      }
    };
  } // namespace impl

  template<typename ... Ts>
  class ptr
  {
    static constexpr std::size_t types_count = sizeof...(Ts);
  public:
    ptr() noexcept :
      x_{ types_count, nullptr }
    {
    }

    ~ptr()
    {
      destroy();
    }

    ptr(const ptr & other) :
      x_{ other.x_.idx, other.apply(impl::copier()) }
    {
    }

    ptr & operator=(const ptr & other)
    {
      if (this != &other)
      {
        clear();
        x_.p = other.apply(impl::copier());
        x_.idx = other.x_.idx;
      }
      return *this;
    }

    ptr(ptr && other) :
      x_{ other.x_.idx, other.x_.p }
    {
      other.x_.idx = types_count;
      other.x_.p = nullptr;
    }

    ptr & operator=(ptr && other) noexcept
    {
      std::size_t tmp_idx = other.x_.idx;
      void * tmp_p = other.x_.p;
      other.x_.idx = types_count;
      other.x_.p = nullptr;
      destroy();
      x_.idx = tmp_idx;
      x_.p = tmp_p;
      return *this;
    }

    template<typename T,
      typename = typename std::enable_if<
        impl::is_variant_contents<
          typename std::remove_const<typename std::remove_reference<T>::type>::type,
          Ts ...
        >::value
        >::type
      >
    explicit ptr(T && other) :
      x_{ impl::index_of_type<T, Ts ...>::value, new T(std::forward<T>(other)) }
    {
    }

    template<typename T,
      typename = typename std::enable_if<
        impl::is_variant_contents<
          typename std::remove_const<typename std::remove_reference<T>::type>::type,
          Ts ...
        >::value
        >::type
      >
    ptr & operator=(T && other)
    {
      if (x_.p != &other)
      {
        if (x_.idx != impl::index_of_type<T, Ts ...>::value)
        {
          clear();
          x_.p = new T(std::forward<T>(other));
          x_.idx = impl::index_of_type<T, Ts ...>::value;
        }
        else
        {
          *static_cast<T*>(x_.p) = std::forward<T>(other);
        }
      }
      return *this;
    }

    template<typename F>
    auto apply(F f)
    {
      return impl::apply<F, Ts ...>::call(0, this->x_, f);
    }

    template<typename F>
    auto apply(F f) const
    {
      return impl::apply<F, const Ts ...>::call(0, this->x_, f);
    }

    template<typename T>
    bool is() const noexcept
    {
      return x_.idx == impl::index_of_type<T, Ts ...>::value;
    }

    bool is_empty() const noexcept
    {
      return nullptr == x_.p;
    }

    template<typename T>
    T & get()
    {
      static_assert(0 <=impl::index_of_type<T, Ts ...>::value, "");
      return *static_cast<T*>(x_.p);
    }

    template<typename T>
    const T & get() const
    {
      static_assert(0 <=impl::index_of_type<T, Ts ...>::value, "");
      return *static_cast<const T*>(x_.p);
    }

    template<typename T>
    T & at()
    {
      if(!is<T>())
      {
        throw std::bad_cast();
      }
      return get<T>();
    }

    template<typename T>
    const T & at() const
    {
      if(!is<T>())
      {
        throw std::bad_cast();
      }
      return get<T>();
    }

    void clear() noexcept
    {
      destroy();
      x_.idx = types_count;
      x_.p = nullptr;
    }

  private:
    void destroy() noexcept
    {
      apply(impl::ref_deleter());
    }

    impl::variant_ptr x_;
  };
} // namespace variant
