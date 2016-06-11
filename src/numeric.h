#pragma once

#include <limits>
#include <typeinfo>

namespace numeric
{
  namespace impl
  {
    template<typename Target, typename Source>
    struct converter
    {
    };

    template<>
    struct converter<unsigned long, long>
    {
      using target_type = unsigned long;
      using source_type = long;

      static target_type convert(source_type source)
      {
        if (source < 0)
        {
          throw std::bad_cast();
        }

        return static_cast<target_type>(source);
      }
    };

    template<>
    struct converter<long, unsigned long>
    {
      using target_type = long;
      using source_type = unsigned long;

      static target_type convert(source_type source)
      {
        if (source > std::numeric_limits<target_type>::max())
        {
          throw std::bad_cast();
        }

        return static_cast<target_type>(source);
      }
    };
  } // namespace impl

  template<typename Target, typename Source>
  Target cast(Source source)
  {
    return impl::converter<Target, Source>::convert(source);
  }
} // namespace numeric
