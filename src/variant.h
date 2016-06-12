#pragma once

#include <utility>
#include <type_traits>

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

    struct variant_ptr
    {
      std::size_t idx;
      void * p;
    };

    template<typename F, typename ... Ts>
    struct apply
    {
      static inline void call(std::size_t, const variant_ptr &, F &)
      {
      }
    };

    template<typename F, typename First, typename ... Ts>
    struct apply<F, First, Ts ...>
    {
      static inline void call(std::size_t idx, const variant_ptr & v, F & f)
      {
        if (v.idx == idx)
        {
          f(*static_cast<First *>(v.p));
        }
        else
        {
          apply<F, Ts ... >::call(idx + 1, v, f);
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
    };
  } // namespace impl

  template<typename ... Ts>
  class ptr
  {
  public:
    template<typename T>
    explicit ptr(const T & other) :
      x_{ impl::index_of_type<T, Ts ...>::value, new T(other) }
    {
    }

    ~ptr()
    {
      apply(impl::ref_deleter());
    }

    template<typename F>
    void apply(F f)
    {
      impl::apply<F, Ts ...>::call(0, this->x_, f);
    }

  private:
    impl::variant_ptr x_;
  };
} // namespace variant
