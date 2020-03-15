#pragma once

#include <climits>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace fibonacci { namespace big_number
{
  using unit = std::uint32_t;
  using double_unit = std::uint64_t;
  constexpr std::size_t unit_bits = CHAR_BIT * sizeof(unit);
  constexpr unit max_unit = std::numeric_limits<unit>::max();
  constexpr double_unit unit_over = ((double_unit)1U << unit_bits);

  using big_digit = std::uint32_t;
  using double_digit = std::uint64_t;
  constexpr big_digit max_big_digit = 999'999'999;
  constexpr big_digit big_digit_over = max_big_digit + 1;
  constexpr std::size_t dec_in_digit = 9;

  big_digit make_digit(char x);
  big_digit from_chars_helper(const char * first, const char * last);
  void to_chars_helper(char * first, char * last, big_digit value);

  std::vector<unit> long_multiplication(const unit * lhs_first, const unit * lhs_last, const unit * rhs_first, const unit * rhs_last);
  std::vector<unit> long_multiplication(const std::vector<unit> & lhs, const std::vector<unit> & rhs);

  std::vector<unit> karatsuba_add_helper(const unit * lhs_first, const unit * lhs_last, const unit * rhs_first, const unit * rhs_last);
  std::vector<unit> karatsuba_add_helper(std::vector<unit> & lhs, const std::vector<unit> & rhs);
  void karatsuba_shift_add_helper(std::vector<unit> & lhs, const std::vector<unit> & rhs, std::size_t rhs_shift);

  std::vector<unit> karatsuba_multiplication(const unit * lhs_first, const unit * lhs_last, const unit * rhs_first, const unit * rhs_last);
  std::vector<unit> karatsuba_multiplication(const std::vector<unit> & lhs, const std::vector<unit> & rhs);

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

    unit divide_by(unit value);
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
    std::vector<big_digit> digits_;

    // default constructor is zero
    unsigned_decimal() noexcept
    {
    };

    unsigned_decimal & operator+=(const unsigned_decimal & rhs);

    big_digit halve();
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

  unsigned_decimal make_unsigned_decimal(unsigned_binary value);

  std::string to_string(const unsigned_decimal & value);
}} // namespace fibonacci::big_number