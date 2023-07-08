#include "../test_lib/test.h"

#include <exception>
#include <optional>
#include <string>
#include <vector>

namespace
{
  template<typename Op>
  void do_two_val_op(std::vector<int>& vals, Op op)
  {
    int x = vals.back();
    vals.pop_back();
    int y = vals.back();
    vals.pop_back();
    vals.push_back(op(x, y));
  }

  std::optional<int> evaluate(std::string_view input)
  {
    std::vector<char> operations;
    std::vector<int> vals;
    auto first = input.cbegin();
    auto last = input.cend();
    while (first != last)
    {
      char crt = *first;
      switch (*first)
      {
        case ' ':
        case '(':
          ++first;
          break;
        case '+':
        case '-':
        case '*':
        case '/':
          operations.push_back(crt);
          ++first;
          break;
        case ')':
          if (operations.empty())
          {
            return std::nullopt;
          }
          ++first;
          {
            char op = operations.back();
            operations.pop_back();
            switch (op)
            {
              case '+':
                  if (vals.size() < 2) return std::nullopt;
                  do_two_val_op(vals, [](int x, int y) { return x + y; });
                  break;
              case '-':
                  if (vals.size() < 2) return std::nullopt;
                  do_two_val_op(vals, [](int x, int y) { return x - y; });
                  break;
              case '*':
                  if (vals.size() < 2) return std::nullopt;
                  do_two_val_op(vals, [](int x, int y) { return x * y; });
                  break;
              case '/':
                  if (vals.size() < 2) return std::nullopt;
                  do_two_val_op(vals, [](int x, int y) { return x / y; });
                  break;
              default:
                std::terminate();
            }
          }
          break;
        default:
          if ((crt < '0') || (crt > '9'))
          {
            return std::nullopt;
          }
          {
            int val = crt - '0';
            ++first;
            for (; first != last; ++first)
            {
              crt = *first;
              if ((crt < '0') || (crt > '9'))
              {
                break;
              }
              val *= 10;
              val += *first;
            }
            vals.push_back(val);
        }
      }
    }
    if (vals.size() != 1)
    {
      return std::nullopt;
    }
    return vals[0];
  }

  TEST(p129_evaluate)
  {
    ASSERT_EQ(2, evaluate("(1+1)"));
  }
} // anonymous namespace