#include "hello.h"

#include <iostream>

namespace fibonacci
{
  int hello(const char * name)
  {
    std::cout << "Hello " << name << '\n';
    return 0;
  }
} // namespace fibonacci