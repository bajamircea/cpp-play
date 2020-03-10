#include "big_number.h"

#include <algorithm>
#include <cassert>
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

  std::vector<unit> long_multiplication(const std::vector<unit> & lhs, const std::vector<unit> & rhs)
  {
    if (lhs.empty())
    {
      return lhs;
    }
    if (rhs.empty())
    {
      return rhs;
    }
    std::vector<unit> total;
    total.resize(lhs.size() + rhs.size());

    for(std::size_t i = 0; i < rhs.size() ; ++i)
    {
      unit carry = 0;
      std::size_t offset = i;
      for(std::size_t j = 0; j < lhs.size() ; ++j, ++offset)
      {
        double_unit tmp = rhs[i];
        tmp *= lhs[j];
        tmp += carry;
        tmp += total[offset];
        total[offset] = tmp;
        carry = tmp >> unit_bits;
      }
      for (;carry != 0; ++offset)
      {
        double_unit tmp = carry;
        tmp += total[offset];
        total[offset] = tmp;
        carry = tmp >> unit_bits;
      }
    }

    std::size_t non_zeroes = total.size();
    while (non_zeroes != 0)
    {
      --non_zeroes;
      if (total[non_zeroes] != 0)
      {
        break;
      }
      total.resize(non_zeroes);
    }

    return total;
  }

  unsigned_binary & unsigned_binary::operator+=(const unsigned_binary & rhs)
  {
    if (rhs.units_.size() <= units_.size())
    {
      unit carry = 0;

      std::size_t i = 0;

      for (; i < rhs.units_.size(); ++i)
      {
        double_unit tmp = units_[i];
        tmp += rhs.units_[i];
        tmp += carry;
        if (tmp >= unit_over)
        {
          units_[i] = tmp - unit_over;
          carry = 1;
        }
        else
        {
          units_[i] = tmp;
          carry = 0;
        }
      }

      if (carry != 0)
      {
        for(; i < units_.size(); ++i)
        {
          if (units_[i] == max_unit)
          {
            units_[i] = 0;
          }
          else
          {
            ++units_[i];
            carry = 0;
            break;
          }
        }

        if (carry != 0)
        {
          units_.push_back(1);
        }
      }
      return *this;
    }
    else //(rhs.digits_.size() > digits_.size())
    {
      unit carry = 0;

      std::size_t i = 0;

      for (; i < units_.size(); ++i)
      {
        double_unit tmp = units_[i];
        tmp += rhs.units_[i];
        tmp += carry;
        if (tmp >= unit_over)
        {
          units_[i] = tmp - unit_over;
          carry = 1;
        }
        else
        {
          units_[i] = tmp;
          carry = 0;
        }
      }

      if (carry != 0)
      {
        for(; i < rhs.units_.size(); ++i)
        {
          if (rhs.units_[i] == max_unit)
          {
            units_.push_back(0);
          }
          else
          {
            units_.push_back(rhs.units_[i] + 1);
            // no more carry
            carry = 0;
            ++i;
            break;
          }
        }
      }

      for(; i < rhs.units_.size(); ++i)
      {
        units_.push_back(rhs.units_[i]);
      }

      if (carry != 0)
      {
        units_.push_back(1);
      }
      return *this;
    }
  }

  unsigned_binary & unsigned_binary::operator-=(const unsigned_binary & rhs)
  {
    assert(*this >= rhs);

    unit carry = 0;

    std::size_t i = 0;

    for (; i < rhs.units_.size(); ++i)
    {
      double_unit a = units_[i];
      double_unit b = rhs.units_[i];
      b += carry;
      if (a < b)
      {
        units_[i] = a + unit_over - b;
        carry = 1;
      }
      else
      {
        units_[i] = a - b;
        carry = 0;
      }
    }

    if (carry != 0)
    {
      for(; i < units_.size(); ++i)
      {
        double_unit a = units_[i];
        if (a == 0)
        {
          units_[i] = unit_over - 1;
        }
        else
        {
          units_[i] -= 1;
          // no more carry
          carry = 0;
          break;
        }
      }
      //assert(0 == carry);
    }

    i = units_.size();
    while (i != 0)
    {
      --i;
      if (units_[i] != 0)
      {
        break;
      }
      units_.resize(i);
    }

    return *this;
  }

  unsigned_binary & unsigned_binary::operator*=(const unsigned_binary & rhs)
  {
    this->units_ = long_multiplication(this->units_, rhs.units_);
    
    return *this;
  }

  unsigned_decimal & unsigned_decimal::operator+=(const unsigned_decimal & rhs)
  {
    if (rhs.digits_.size() <= digits_.size())
    {
      digit carry = 0;

      std::size_t i = 0;

      for (; i < rhs.digits_.size(); ++i)
      {
        digits_[i] += rhs.digits_[i] + carry;
        if (digits_[i] >= 10)
        {
          digits_[i] -= 10;
          carry = 1;
        }
        else
        {
          carry = 0;
        }
      }

      if (carry != 0)
      {
        for(; i < digits_.size(); ++i)
        {
          if (digits_[i] == 9)
          {
            digits_[i] = 0;
          }
          else
          {
            ++digits_[i];
            carry = 0;
            break;
          }
        }

        if (carry != 0)
        {
          digits_.push_back(1);
        }
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
        if (digits_[i] >= 10)
        {
          digits_[i] -= 10;
          carry = 1;
        }
        else
        {
          carry = 0;
        }
      }

      if (carry != 0)
      {
        for(; i < rhs.digits_.size(); ++i)
        {
          if (rhs.digits_[i] == 9)
          {
            digits_.push_back(0);
          }
          else
          {
            digits_.push_back(rhs.digits_[i] + 1);
            // no more carry
            carry = 0;
            ++i;
            break;
          }
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