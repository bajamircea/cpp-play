#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace fibonacci { namespace big_number
{
  using unit = std::uint32_t;
  using double_unit = std::uint64_t;
  using digit = unsigned char;
  constexpr std::size_t unit_bits = 8U * sizeof(unit);
  constexpr double_unit unit_over = ((double_unit)1U << unit_bits);
  constexpr unit max_unit = std::numeric_limits<unit>::max();

  digit make_digit(char x);
  char from_digit(digit x);

  std::vector<unit> long_multiplication(const std::vector<unit> & lhs, const std::vector<unit> & rhs);

  struct unsigned_binary
  {
    std::vector<unit> units_;

    // default constructor is zero
    unsigned_binary() noexcept
    {
    };

    explicit unsigned_binary(unit value)
    {
      if (0 == value)
      {
        return;
      }
      units_.push_back(value);
    }

    unsigned_binary & operator+=(const unsigned_binary & rhs);

    unsigned_binary & operator-=(const unsigned_binary & rhs);

    unsigned_binary & operator*=(const unsigned_binary & rhs);
  };

  inline bool operator==(unsigned_binary lhs, const unsigned_binary & rhs)
  {
    return lhs.units_ == rhs.units_;
  }

  inline bool operator!=(unsigned_binary lhs, const unsigned_binary & rhs)
  {
    return !(lhs == rhs);
  }

  inline bool operator<(unsigned_binary lhs, const unsigned_binary & rhs)
  {
    if (lhs.units_.size() < rhs.units_.size())
    {
      return true;
    }
    if (rhs.units_.size() < lhs.units_.size())
    {
      return false;
    }
    for (size_t i = lhs.units_.size(); i != 0; )
    {
      --i;
      if(lhs.units_[i] < rhs.units_[i])
      {
        return true;
      }
      if(rhs.units_[i] < lhs.units_[i])
      {
        return false;
      }
    }
    return false;
  }

  inline bool operator<=(unsigned_binary lhs, const unsigned_binary & rhs)
  {
    return !(rhs < lhs);
  }

  inline bool operator>(unsigned_binary lhs, const unsigned_binary & rhs)
  {
    return rhs < lhs;
  }

  inline bool operator>=(unsigned_binary lhs, const unsigned_binary & rhs)
  {
    return !(lhs < rhs);
  }

  inline unsigned_binary operator+(unsigned_binary lhs, const unsigned_binary & rhs)
  {
    lhs += rhs;
    return lhs;
  }

  inline unsigned_binary operator-(unsigned_binary lhs, const unsigned_binary & rhs)
  {
    lhs -= rhs;
    return lhs;
  }

  inline unsigned_binary operator*(unsigned_binary lhs, const unsigned_binary & rhs)
  {
    lhs *= rhs;
    return lhs;
  }

  struct unsigned_decimal
  {
    std::vector<digit> digits_;

    // default constructor is zero
    unsigned_decimal() noexcept
    {
    };

    unsigned_decimal & operator+=(const unsigned_decimal & rhs);

    digit halve();
  };

  inline bool operator==(unsigned_decimal lhs, const unsigned_decimal & rhs)
  {
    return lhs.digits_ == rhs.digits_;
  }

  inline bool operator!=(unsigned_decimal lhs, const unsigned_decimal & rhs)
  {
    return !(lhs == rhs);
  }

  inline bool operator<(unsigned_decimal lhs, const unsigned_decimal & rhs)
  {
    if (lhs.digits_.size() < rhs.digits_.size())
    {
      return true;
    }
    if (rhs.digits_.size() < lhs.digits_.size())
    {
      return false;
    }
    for (size_t i = lhs.digits_.size(); i != 0; )
    {
      --i;
      if(lhs.digits_[i] < rhs.digits_[i])
      {
        return true;
      }
      if(rhs.digits_[i] < lhs.digits_[i])
      {
        return false;
      }
    }
    return false;
  }

  inline bool operator<=(unsigned_decimal lhs, const unsigned_decimal & rhs)
  {
    return !(rhs < lhs);
  }

  inline bool operator>(unsigned_decimal lhs, const unsigned_decimal & rhs)
  {
    return rhs < lhs;
  }

  inline bool operator>=(unsigned_decimal lhs, const unsigned_decimal & rhs)
  {
    return !(lhs < rhs);
  }

  inline unsigned_decimal operator+(unsigned_decimal lhs, const unsigned_decimal & rhs)
  {
    lhs += rhs;
    return lhs;
  }

  unsigned_binary make_unsigned_binary(unsigned_decimal value);

  unsigned_binary make_unsigned_binary(const char * first, std::size_t count);

  template <std::size_t N>
  unsigned_binary make_unsigned_binary(const char (&string_literal)[N])
  {
    static_assert(N > 0, "");
    return make_unsigned_binary(string_literal, N - 1);
  }

  std::string to_string(const unsigned_binary & value);

  unsigned_decimal make_unsigned_decimal(const char * first, std::size_t count);

  template <std::size_t N>
  unsigned_decimal make_unsigned_decimal(const char (&string_literal)[N])
  {
    static_assert(N > 0, "");
    return make_unsigned_decimal(string_literal, N - 1);
  }

  unsigned_decimal make_unsigned_decimal(const unsigned_binary & value);

  std::string to_string(const unsigned_decimal & value);
}} // namespace fibonacci::big_number