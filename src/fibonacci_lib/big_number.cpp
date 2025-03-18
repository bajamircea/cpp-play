#include "big_number.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <type_traits>

namespace
{
  const std::size_t g_karatsuba_threshold = 100;
}

namespace fibonacci { namespace big_number
{
  static_assert(2*sizeof(unit) == sizeof(double_unit), "");

  static_assert(std::is_unsigned<unit>::value ,"");

  static_assert(std::is_unsigned<double_unit>::value, "");

  static_assert(std::is_unsigned<big_digit>::value, "");

  static_assert(std::is_unsigned<double_digit>::value, "");

  big_digit make_digit(char x)
  {
    return x - '0';
  }

  big_digit from_chars_helper(const char * first, const char * last)
  {
    big_digit return_value = 0;
    switch(last - first)
    {
      case 9:
        static_assert(9 == dec_in_digit, "");
        return_value += make_digit(*first) * 100'000'000;
        ++first;
        [[fallthrough]];
      case 8:
        return_value += make_digit(*first) * 10'000'000;
        ++first;
        [[fallthrough]];
      case 7:
        return_value += make_digit(*first) * 1'000'000;
        ++first;
        [[fallthrough]];
      case 6:
        return_value += make_digit(*first) * 100'000;
        ++first;
        [[fallthrough]];
      case 5:
        return_value += make_digit(*first) * 10'000;
        ++first;
        [[fallthrough]];
      case 4:
        return_value += make_digit(*first) * 1'000;
        ++first;
        [[fallthrough]];
      case 3:
        return_value += make_digit(*first) * 100;
        ++first;
        [[fallthrough]];
      case 2:
        return_value += make_digit(*first) * 10;
        ++first;
        [[fallthrough]];
      case 1:
        return_value += make_digit(*first);
    }
    return return_value;
  }

  void to_chars_helper(char * first, char * last, big_digit value)
  {
    static constexpr char pairs[201] =
      "0001020304050607080910111213141516171819"
      "2021222324252627282930313233343536373839"
      "4041424344454647484950515253545556575859"
      "6061626364656667686970717273747576777879"
      "8081828384858687888990919293949596979899";
    while (value >= 100)
    {
      auto offset = (value % 100) * 2;
      value /= 100;
      --last;
      *last = pairs[offset + 1];
      --last;
      *last = pairs[offset];
    }
    if (value >= 10)
    {
      auto offset = value * 2;
      --last;
      *last = pairs[offset + 1];
      --last;
      *last = pairs[offset];
    }
    else
    {
      --last;
      *last = '0' + value;
    }
    while (first != last)
    {
      --last;
      *last = '0';
    }
  }

  std::vector<unit> long_multiplication(const unit * lhs_first, const unit * lhs_last, const unit * rhs_first, const unit * rhs_last)
  {
    std::vector<unit> total;

    if ((lhs_first == lhs_last) || (rhs_first == rhs_last))
    {
      return total;
    }
    std::size_t lhs_size = lhs_last - lhs_first;
    std::size_t rhs_size = rhs_last - rhs_first;
    total.resize(lhs_size + rhs_size);

    for(std::size_t i = 0; i < rhs_size ; ++i)
    {
      unit carry = 0;
      std::size_t offset = i;
      for(std::size_t j = 0; j < lhs_size ; ++j, ++offset)
      {
        double_unit tmp = rhs_first[i];
        tmp *= lhs_first[j];
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

  std::vector<unit> long_multiplication(const std::vector<unit> & lhs, const std::vector<unit> & rhs)
  {
    return long_multiplication(lhs.data(), lhs.data() + lhs.size(), rhs.data(), rhs.data() + rhs.size());
  }

  std::vector<unit> karatsuba_add_helper(const unit * lhs_first, const unit * lhs_last, const unit * rhs_first, const unit * rhs_last)
  {
    std::vector<unit> return_value;
    
    unit carry = 0;

    for (;(lhs_first != lhs_last) && (rhs_first != rhs_last); ++lhs_first, ++rhs_first)
    {
      double_unit tmp = *lhs_first;
      tmp += *rhs_first;
      tmp += carry;
      if (tmp >= unit_over)
      {
        return_value.push_back(tmp - unit_over);
        carry = 1;
      }
      else
      {
        return_value.push_back(tmp);
        carry = 0;
      }
    }

    const unit * x_first = (lhs_first != lhs_last) ? lhs_first : rhs_first;
    const unit * x_last = (lhs_first != lhs_last) ? lhs_last : rhs_last;

    if (carry != 0)
    {
      for(; x_first != x_last; ++x_first)
      {
        if (*x_first == max_unit)
        {
          return_value.push_back(0);
        }
        else
        {
          return_value.push_back(*x_first + 1);
          carry = 0;
          ++x_first;
          break;
        }
      }
    }

    for(; x_first != x_last; ++x_first)
    {
      return_value.push_back(*x_first);
    }

    if (carry != 0)
    {
      return_value.push_back(1);
    }

    return return_value;
  }

  std::vector<unit> karatsuba_add_helper(std::vector<unit> & lhs, const std::vector<unit> & rhs)
  {
    return karatsuba_add_helper(lhs.data(), lhs.data() + lhs.size(), rhs.data(), rhs.data() + rhs.size());
  }

  void karatsuba_shift_add_helper(std::vector<unit> & lhs, const std::vector<unit> & rhs, std::size_t rhs_shift)
  {
    if (rhs.empty())
    {
      return;
    }

    if (lhs.size() < rhs_shift)
    {
      lhs.resize(rhs_shift);
    }

    if (lhs.size() >= rhs.size() + rhs_shift)
    {
      unit carry = 0;

      std::size_t i = rhs_shift;
      std::size_t j = 0;

      for (; j < rhs.size(); ++i, ++j)
      {
        double_unit tmp = lhs[i];
        tmp += rhs[j];
        tmp += carry;
        if (tmp >= unit_over)
        {
          lhs[i] = tmp - unit_over;
          carry = 1;
        }
        else
        {
          lhs[i] = tmp;
          carry = 0;
        }
      }

      if (carry != 0)
      {
        for(; i < lhs.size(); ++i)
        {
          if (lhs[i] == max_unit)
          {
            lhs[i] = 0;
          }
          else
          {
            ++lhs[i];
            carry = 0;
            break;
          }
        }

        if (carry != 0)
        {
          lhs.push_back(1);
        }
      }
    }
    else //lhs.size() < rhs.size() + rhs_shift
    {
      unit carry = 0;

      std::size_t i = rhs_shift;
      std::size_t j = 0;

      for (; i < lhs.size(); ++i, ++j)
      {
        double_unit tmp = lhs[i];
        tmp += rhs[j];
        tmp += carry;
        if (tmp >= unit_over)
        {
          lhs[i] = tmp - unit_over;
          carry = 1;
        }
        else
        {
          lhs[i] = tmp;
          carry = 0;
        }
      }

      if (carry != 0)
      {
        for(; j < rhs.size(); ++j)
        {
          if (rhs[j] == max_unit)
          {
            lhs.push_back(0);
          }
          else
          {
            lhs.push_back(rhs[j] + 1);
            // no more carry
            carry = 0;
            ++j;
            break;
          }
        }
      }

      for(; j < rhs.size(); ++j)
      {
        lhs.push_back(rhs[j]);
      }

      if (carry != 0)
      {
        lhs.push_back(1);
      }
    }
  }

  std::vector<unit> karatsuba_multiplication(const unit * lhs_first, const unit * lhs_last, const unit * rhs_first, const unit * rhs_last)
  {
    std::size_t lhs_size = lhs_last - lhs_first;
    std::size_t rhs_size = rhs_last - rhs_first;
    if (lhs_size < g_karatsuba_threshold || rhs_size < g_karatsuba_threshold)
    {
      return long_multiplication(lhs_first, lhs_last, rhs_first, rhs_last);
    }

    // x = x1*B^m + x0
    // y = y1*B^m + y1
    // z0 = x0 * y0
    // z2 = x1 * y1
    // z1 = (x1 + x0)*(y1 + y0) - z2 - z0
    // x * y = z2*B^2m + z1*B^m + z0

    std::size_t split_size = std::max(lhs_size, rhs_size) / 2;
    const unit * lhs_split = (lhs_size > split_size) ? (lhs_first + split_size) : lhs_last;
    const unit * rhs_split = (rhs_size > split_size) ? (rhs_first + split_size) : rhs_last;

    unsigned_binary z0;
    z0.units_ = karatsuba_multiplication(lhs_first, lhs_split, rhs_first, rhs_split);
    unsigned_binary z2;
    z2.units_ = karatsuba_multiplication(lhs_split, lhs_last, rhs_split, rhs_last);

    std::vector<unit> p1 = karatsuba_add_helper(lhs_first, lhs_split, lhs_split, lhs_last);
    std::vector<unit> p2 = karatsuba_add_helper(rhs_first, rhs_split, rhs_split, rhs_last);
    unsigned_binary z1;
    z1.units_ = karatsuba_multiplication(p1, p2);
    z1 -= z0;
    z1 -= z2;

    std::vector<unit> return_value = std::move(z0.units_);
    karatsuba_shift_add_helper(return_value, z1.units_, split_size);
    karatsuba_shift_add_helper(return_value, z2.units_, 2 * split_size);
    return return_value;
  }

  std::vector<unit> karatsuba_multiplication(const std::vector<unit> & lhs, const std::vector<unit> & rhs)
  {
    return karatsuba_multiplication(lhs.data(), lhs.data() + lhs.size(), rhs.data(), rhs.data() + rhs.size());
  }

  unsigned_binary & unsigned_binary::operator+=(const unsigned_binary & rhs)
  {
    if (units_.size() >= rhs.units_.size())
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
    else // units_.size() < rhs.units_.size())
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
    this->units_ = karatsuba_multiplication(this->units_, rhs.units_);
    
    return *this;
  }

  unit unsigned_binary::divide_by(unit value)
  {
    unit carry = 0;

    if (units_.empty())
    {
      return carry;
    }

    std::size_t i = units_.size() - 1;
    carry = units_[i] % value;
    if (units_[i] < value)
    {
      units_.pop_back();
    }
    else
    {
      units_[i] /= value;
    }

    while (i != 0)
    {
      --i;
      double_unit tmp;
      if (carry != 0)
      {
        tmp = carry;
        tmp *= unit_over;
        tmp += units_[i]; 
      }
      else
      {
        tmp = units_[i];
      }
      
      carry = tmp % value;
      units_[i] = tmp / value;
    }

    return carry;
  }

  unsigned_decimal & unsigned_decimal::operator+=(const unsigned_decimal & rhs)
  {
    if (digits_.size() >= rhs.digits_.size())
    {
      big_digit carry = 0;

      std::size_t i = 0;

      for (; i < rhs.digits_.size(); ++i)
      {
        double_digit tmp = digits_[i];
        tmp += rhs.digits_[i];
        tmp += carry;
        if (tmp >= big_digit_over)
        {
          digits_[i] = tmp - big_digit_over;
          carry = 1;
        }
        else
        {
          digits_[i] = tmp;
          carry = 0;
        }
      }

      if (carry != 0)
      {
        for(; i < digits_.size(); ++i)
        {
          if (digits_[i] == max_big_digit)
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
    else // digits_.size() < rhs.digits_.size()
    {
      big_digit carry = 0;

      std::size_t i = 0;

      for (; i < digits_.size(); ++i)
      {
        double_digit tmp = digits_[i];
        tmp += rhs.digits_[i];
        tmp += carry;
        if (tmp >= big_digit_over)
        {
          digits_[i] = tmp - big_digit_over;
          carry = 1;
        }
        else
        {
          digits_[i] = tmp;
          carry = 0;
        }
      }

      if (carry != 0)
      {
        for(; i < rhs.digits_.size(); ++i)
        {
          if (rhs.digits_[i] == max_big_digit)
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

  big_digit unsigned_decimal::halve()
  {
    big_digit carry = 0;

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
      double_digit x = digits_[i];
      if (carry != 0)
      {
        x += big_digit_over;
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
      big_digit carry = value.halve();
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

    const char * last = first + count;
    first = std::find_if(first, last, [](char x){ return x != '0';});
    count = last - first;

    std::size_t whole_digits = count / dec_in_digit;
    std::size_t remaining = count % dec_in_digit;
    
    const char * first_group = first;
    const char * last_group;
    if (remaining != 0)
    {
      return_value.digits_.resize(whole_digits + 1);
      last_group = first_group + remaining;
    }
    else
    {
      return_value.digits_.resize(whole_digits);
      if (whole_digits == 0)
      {
        return return_value;
      }
      last_group = first_group + dec_in_digit;
    }
    std::size_t i = return_value.digits_.size();
    while (true)
    {
      --i;
      return_value.digits_[i] = from_chars_helper(first_group, last_group);
      if (last_group == last)
      {
        break;
      }
      first_group = last_group;
      last_group += dec_in_digit;
    }

    return return_value;
  }

  unsigned_decimal make_unsigned_decimal(unsigned_binary value)
  {
    unsigned_decimal return_value;

    while (!value.units_.empty())
    {
      big_digit carry = value.divide_by(big_digit_over);
      return_value.digits_.push_back(carry);
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

    std::size_t first_size;
    std::size_t i = value.digits_.size() - 1;
    if(value.digits_[i] < 10)
    {
      first_size = 1;
    }
    else if(value.digits_[i] < 100)
    {
      first_size = 2;
    }
    else if(value.digits_[i] < 1000)
    {
      first_size = 3;
    }
    else if(value.digits_[i] < 10'000)
    {
      first_size = 4;
    }
    else if(value.digits_[i] < 100'000)
    {
      first_size = 5;
    }
    else if(value.digits_[i] < 1'000'000)
    {
      first_size = 6;
    }
    else if(value.digits_[i] < 10'000'000)
    {
      first_size = 7;
    }
    else if(value.digits_[i] < 100'000'000)
    {
      first_size = 8;
    }
    else
    {
      first_size = 9;
      static_assert(9 == dec_in_digit, "");
    }

    return_value.resize(first_size + dec_in_digit * i);
    char * first = &return_value[0];
    char * last = first + first_size;

    while(true)
    {
      to_chars_helper(first, last, value.digits_[i]);
      if (i == 0)
      {
        break;
      }
      --i;
      first = last;
      last += dec_in_digit;
    }

    return return_value;
  }
}} // namespace fibonacci::big_number