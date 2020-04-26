#include <array>
#include <iostream>
#include <vector>

namespace
{
  enum operation
  {
    default_constructor,
    other_constructor,
    copy_constructor,
    copy_assignment,
    move_constructor,
    move_assignment,
    destructor,
    max_operation,
  };

  std::array<int, operation::max_operation> g_counters;
  const std::array<const char *, operation::max_operation> g_labels = {"c", "o", "cc", "c=", "mc", "ma", "~"};

  void reset()
  {
    for (int & x : g_counters)
    {
      x = 0;
    }
  }

  void print_and_reset()
  {
    const char * sep = "";
    for (int i = 0; i < operation::max_operation; ++i)
    {
      if (g_counters[i] != 0)
      {
        std::cout << sep << g_labels[i] << ": " << g_counters[i];
        sep = ", ";
      }
    }
    std::cout << "\n";
    reset();
  }

  struct instrumented_simple
  {
    int i_;
    instrumented_simple() noexcept:i_{0} { ++g_counters[operation::default_constructor]; }
    explicit instrumented_simple(int i) noexcept:i_{i} { ++g_counters[operation::other_constructor]; }
    instrumented_simple(const instrumented_simple & other) noexcept:i_{other.i_} { ++g_counters[operation::copy_constructor]; }
    instrumented_simple & operator=(const instrumented_simple & other) noexcept { i_ = other.i_; ++g_counters[operation::copy_assignment]; return *this; }
    instrumented_simple(instrumented_simple && other) noexcept:i_{other.i_} { ++g_counters[operation::move_constructor]; }
    instrumented_simple & operator=(instrumented_simple && other) noexcept { i_ = other.i_; ++g_counters[operation::move_assignment]; return *this; }
    ~instrumented_simple() { ++g_counters[operation::destructor]; }
  };

  struct instrumented_container
  {
    int * pi_;
    instrumented_container() noexcept:pi_{0} { ++g_counters[operation::default_constructor]; }
    explicit instrumented_container(int i):pi_{new int(i)} { ++g_counters[operation::other_constructor]; }
    instrumented_container(const instrumented_container & other):pi_{new int(*other.pi_)} { ++g_counters[operation::copy_constructor]; }
    instrumented_container & operator=(const instrumented_container & other) { delete pi_; pi_ = new int(*other.pi_); ++g_counters[operation::copy_assignment]; return *this; }
    instrumented_container(instrumented_container && other) noexcept:pi_{other.pi_} { other.pi_ = nullptr; ++g_counters[operation::move_constructor]; }
    instrumented_container & operator=(instrumented_container && other) noexcept { delete pi_; pi_ = other.pi_; other.pi_ = nullptr; ++g_counters[operation::move_assignment]; return *this; }
    ~instrumented_container() { delete pi_; ++g_counters[operation::destructor]; }
  };

  struct instrumented_sentinel
  {
    int * pi_;
    instrumented_sentinel() noexcept:pi_{0} { ++g_counters[operation::default_constructor]; }
    explicit instrumented_sentinel(int i):pi_{new int(i)} { ++g_counters[operation::other_constructor]; }
    instrumented_sentinel(const instrumented_sentinel & other):pi_{new int(*other.pi_)} { ++g_counters[operation::copy_constructor]; }
    instrumented_sentinel & operator=(const instrumented_sentinel & other) { delete pi_; pi_ = new int(*other.pi_); ++g_counters[operation::copy_assignment]; return *this; }
    instrumented_sentinel(instrumented_sentinel && other):pi_{new int(*other.pi_)} { ++g_counters[operation::move_constructor]; }
    instrumented_sentinel & operator=(instrumented_sentinel && other) noexcept { std::swap(pi_, other.pi_); ++g_counters[operation::move_assignment]; return *this; }
    ~instrumented_sentinel() { delete pi_; ++g_counters[operation::destructor]; }
  };

  template<typename T>
  void push_back_test(const char * message)
  {
    std::cout << "==== " << message << "\n";

    reset();

    std::vector<T> x;

    for (int i = 0 ; i < 10; ++i)
    {
      x.push_back(T(i));
      print_and_reset();
      std::cout << "size: " << x.size() << ", capacity: " << x.capacity() << "\n";
    }
  }

  void fill_test()
  {
    std::cout << "==== fill test\n";

    std::vector<int> x(0);

    double usage = 0;

    int n = 1024 * 1024;

    for (int i = 0 ; i < n; ++i)
    {
      x.push_back(i);
      double this_usage = double(x.size()) / double(x.capacity());
      //std::cout << "usage: " << this_usage << "\n";
      //std::cout << "size: " << x.size() << ", capacity: " << x.capacity() << "\n";
      usage += this_usage;
    }

    std::cout << "average usage: " << (usage / double(n)) << "\n";
  }
}

int main()
{
  push_back_test<instrumented_simple>("simple type");
  push_back_test<instrumented_container>("simple container");
  push_back_test<instrumented_container>("container with dynamically allocated sentinel");
  fill_test();
  std::cout << "Done!\n";
}