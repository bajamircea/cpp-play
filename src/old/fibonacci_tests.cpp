#include "fibonacci.h"

#include "test.h"

#include <iostream>

namespace
{
  // workaround for g++ not supporting yet std::size
  template<class T, size_t N>
  size_t std_size(T (&)[N]) { return N; }

  using namespace fibonacci;

  struct
  {
    int input;
    int expected;
  } params[] = 
  {
    { 0, 0 },
    { 1, 1 },
    { 2, 1 },
    { 6, 8 },
    //{ 41, 165580141 },
    //{ 42, 267914296 },
  };

  TEST(fibonacci__recurse)
  {
    for (size_t i = 0 ; i < std_size(params) ; ++i)
    {
      const auto & data = params[i];
      int actual = recurse(data.input);
      if (data.expected != actual)
      {
        std::cout << "Expected fibonacci(" << data.input << "): " <<
        data.expected << ". Actual: " << actual << std::endl;
        FAIL();
      }
    }
  }

  TEST(fibonacci__linear)
  {
    for (size_t i = 0 ; i < std_size(params) ; ++i)
    {
      const auto & data = params[i];
      int actual = linear(data.input);
      if (data.expected != actual)
      {
        std::cout << "Expected fibonacci(" << data.input << "): " <<
        data.expected << ". Actual: " << actual << std::endl;
        FAIL();
      }
    }
  }
} // anonymous namespace
