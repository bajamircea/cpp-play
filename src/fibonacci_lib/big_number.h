#pragma once

// This defines a large number as a vector of unsigned

#include <cstdint>
#include <string>
#include <vector>

namespace fibonacci { namespace big_number
{
  using digit = unsigned char;
  using unit = std::uint32_t;
  using double_unit = std::uint64_t;

  digit make_digit(char x);
  char from_digit(digit x);

  struct unsigned_binary
  {
    std::vector<unit> units_;
  };

  inline bool operator==(unsigned_binary lhs, const unsigned_binary& rhs)
  {
    return lhs.units_ == rhs.units_;
  }

  inline bool operator!=(unsigned_binary lhs, const unsigned_binary& rhs)
  {
    return !(lhs == rhs);
  }

  struct unsigned_decimal
  {
    std::vector<digit> digits_;

    // default constructor is zero
    unsigned_decimal() noexcept;

    unsigned_decimal& operator+=(const unsigned_decimal& rhs);
  };

  inline unsigned_decimal operator+(unsigned_decimal lhs, const unsigned_decimal& rhs)
  {
    lhs += rhs;
    return lhs;
  }

  inline bool operator==(unsigned_decimal lhs, const unsigned_decimal& rhs)
  {
    return lhs.digits_ == rhs.digits_;
  }

  inline bool operator!=(unsigned_decimal lhs, const unsigned_decimal& rhs)
  {
    return !(lhs == rhs);
  }

  inline bool operator<(unsigned_decimal lhs, const unsigned_decimal& rhs)
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

  inline bool operator<=(unsigned_decimal lhs, const unsigned_decimal& rhs)
  {
    return !(rhs < lhs);
  }

  inline bool operator>(unsigned_decimal lhs, const unsigned_decimal& rhs)
  {
    return rhs < lhs;
  }

  inline bool operator>=(unsigned_decimal lhs, const unsigned_decimal& rhs)
  {
    return !(lhs < rhs);
  }

  unsigned_decimal make_unsigned_decimal(const char * first, std::size_t count);

  template <std::size_t N>
  unsigned_decimal make_unsigned_decimal(const char (&string_literal)[N])
  {
    static_assert(N > 0, "");
    return make_unsigned_decimal(string_literal, N - 1);
  }

  std::string to_string(const unsigned_decimal & value);
}} // namespace fibonacci::big_number