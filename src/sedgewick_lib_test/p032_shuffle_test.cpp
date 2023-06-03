#include "../test_lib/test.h"

#include <cstddef>
#include <iterator>
#include <random>
#include <vector>

namespace
{
  class random_number_generator
  {
    std::random_device dev;
    std::mt19937_64 generator{ dev() };
  public:
    template<std::integral T>
    T uniform(T max)
    {
      return uniform(T(), max);
    }

    template<std::integral T>
    T uniform(T min, T max)
    {
      std::uniform_int_distribution<T> dist(min, max);
      return dist(generator);
    }
  };

  template<std::bidirectional_iterator I, typename Generator>
  void shuffle(I first, I last, Generator & generator)
  {
    if (first == last)
    {
      return;
    }
    for(--last;first != last; ++first)
    {
      std::size_t n = last - first;
      std::size_t other = generator.uniform(n);
      if (other != 0)
      {
        std::iter_swap(first, first + n);
      }
    }
  }

  TEST(p032_shuffle)
  {
    int data[] = {101, 102, 103, 104};

    struct dummy_generator
    {
      std::vector<size_t> inputs{ 1, 2, 3};
      std::size_t uniform(std::size_t max)
      {
        ASSERT_FALSE(inputs.empty());
        ASSERT_EQ(inputs.back(), max);
        inputs.pop_back();
        return max;
      }
    };

    dummy_generator dng;
    shuffle(std::begin(data), std::end(data), dng);
    ASSERT_EQ(104, data[0]);
    ASSERT_EQ(101, data[1]);
    ASSERT_EQ(102, data[2]);
    ASSERT_EQ(103, data[3]);


    random_number_generator rng;
    shuffle(std::begin(data), std::end(data), rng);
  }
} // anonymous namespace