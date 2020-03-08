#include "big_number.h"

#include <algorithm>
#include <iterator>
#include <type_traits>

namespace fibonacci { namespace big_number
{
  static_assert(2*sizeof(unit) == sizeof(double_unit), "");

  static_assert(std::is_unsigned<unit>::value ,"");

  static_assert(std::is_unsigned<double_unit>::value, "");

  static_assert(std::is_unsigned<digit>::value, "");

  digit make_digit(char x)
  {
    return x - '0';
  }

  char from_digit(digit x)
  {
    return '0' + x;
  }

  unsigned_decimal& unsigned_decimal::operator+=(const unsigned_decimal & rhs)
  {
    if (rhs.digits_.size() <= digits_.size())
    {
      digit carry = 0;

      std::size_t i = 0;

      for (; i < rhs.digits_.size(); ++i)
      {
        digits_[i] += rhs.digits_[i] + carry;
        if (digits_[i] > 9)
        {
          digits_[i] -= 10;
          carry = 1;
        }
        else
        {
          carry = 0;
        }
      }

      if (carry == 0)
      {
          // no more carry
          return *this;
      }

      for(; i < digits_.size(); ++i)
      {
        digits_[i] += carry;
        if (digits_[i] > 9)
        {
          digits_[i] -= 10;
          carry = 1;
        }
        else
        {
          // no more carry
          return *this;
        }
      }

      if (carry != 0)
      {
        digits_.push_back(1);
      }
      return *this;
    }
    else //(rhs.digits_.size() > digits_.size())
    {
      digit carry = 0;

      std::size_t i = 0;

      for (; i < digits_.size(); ++i)
      {
        digits_[i] += rhs.digits_[i] + carry;
        if (digits_[i] > 9)
        {
          digits_[i] -= 10;
          carry = 1;
        }
        else
        {
          carry = 0;
        }
      }

      for(; i < rhs.digits_.size(); ++i)
      {
        digit x = rhs.digits_[i] + carry;
        if (x > 9)
        {
          digits_.push_back(x - 10);
          carry = 1;
        }
        else
        {
          digits_.push_back(x);
          // no more carry
          carry = 0;
          ++i;
          break;
        }
      }

      for(; i < rhs.digits_.size(); ++i)
      {
        digits_.push_back(rhs.digits_[i]);
      }

      if (carry != 0)
      {
        digits_.push_back(1);
      }
      return *this;
    }
  }

  digit unsigned_decimal::halve()
  {
    digit carry = 0;

    if (digits_.empty())
    {
      return carry;
    }

    std::size_t i = digits_.size() - 1;
    carry = digits_[i] & 1U;
    if (digits_[i] < 2)
    {
      digits_.pop_back();
    }
    else
    {
      digits_[i] >>= 1;
    }

    while (i != 0)
    {
      --i;
      digit x = digits_[i];
      if (carry != 0)
      {
        x += 10;
      }
      carry = digits_[i] & 1U;
      digits_[i] = x >> 1;
    }

    return carry;
  }

  unsigned_binary make_unsigned_binary(unsigned_decimal value)
  {
    unsigned_binary return_value;

    unit mask = 1U;
    unit current = 0U;
    size_t bit_pos = 0;

    while (!value.digits_.empty())
    {
      digit carry = value.halve();
      if (carry != 0)
      {
        current |= mask;
      }
      ++bit_pos;
      if (bit_pos == unit_bits)
      {
        return_value.units_.push_back(current);
        mask = 1U;
        current = 0U;
        bit_pos = 0;
      }
      else
      {
        mask <<= 1;
      }
    }

    if (current != 0)
    {
      return_value.units_.push_back(current);
    }

    return return_value;
  }

  unsigned_binary make_unsigned_binary(const char * first, std::size_t count)
  {
    return make_unsigned_binary(make_unsigned_decimal(first, count));
  }

  std::string to_string(const unsigned_binary & value)
  {
    return to_string(make_unsigned_decimal(value));
  }

  unsigned_decimal make_unsigned_decimal(const char * first, std::size_t count)
  {
    unsigned_decimal return_value;

    auto first_non_zero = std::find_if(first, first + count, [](char x){ return x != '0';});
    return_value.digits_.resize(first + count - first_non_zero);

    auto rfirst = std::make_reverse_iterator(first + count);
    auto rlast = std::make_reverse_iterator(first_non_zero);

    for(std::size_t i = 0; rfirst != rlast; ++rfirst, ++i)
    {
      return_value.digits_[i] = make_digit(*rfirst);
    }

    return return_value;
  }

  unsigned_decimal make_unsigned_decimal(const unsigned_binary & value)
  {
    unsigned_decimal return_value;

    unsigned_decimal power_of_2;
    power_of_2.digits_.push_back(1);

    for(unit u : value.units_)
    {
      unit mask = 1U;
      for(size_t bit_pos = 0; bit_pos < unit_bits; ++bit_pos)
      {
        if (mask & u)
        {
          return_value += power_of_2;
        }
        mask <<= 1;
        power_of_2 += power_of_2;
      }
    }

    return return_value;
  }

  std::string to_string(const unsigned_decimal & value)
  {
    std::string return_value;

    if (value.digits_.empty())
    {
      return_value.push_back('0');
      return return_value;
    }

    return_value.resize(value.digits_.size());

    auto rfirst = value.digits_.rbegin();
    auto rlast = value.digits_.rend();

    for(std::size_t i = 0; rfirst != rlast; ++rfirst, ++i)
    {
      return_value[i] = from_digit(*rfirst);
    }

    return return_value;
  }
}} // namespace fibonacci::big_number