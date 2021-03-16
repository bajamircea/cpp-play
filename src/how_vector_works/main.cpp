#include <array>
#include <iostream>
#include <list>
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

  struct instrumented_copy_only
  {
    int i_;
    instrumented_copy_only() noexcept:i_{0} { ++g_counters[operation::default_constructor]; }
    explicit instrumented_copy_only(int i) noexcept:i_{i} { ++g_counters[operation::other_constructor]; }
    instrumented_copy_only(const instrumented_copy_only & other) noexcept:i_{other.i_} { ++g_counters[operation::copy_constructor]; }
    instrumented_copy_only & operator=(const instrumented_copy_only & other) noexcept { i_ = other.i_; ++g_counters[operation::copy_assignment]; return *this; }
    ~instrumented_copy_only() { ++g_counters[operation::destructor]; }
  };

  template<typename T>
  void push_back_test(const char * message, int n)
  {
    std::cout << "==== " << message << "\n";

    reset();

    std::vector<T> x;

    for (int i = 0 ; i < n; ++i)
    {
      x.push_back(T(i));
      print_and_reset();
      std::cout << "size: " << x.size() << ", capacity: " << x.capacity() << "\n";
    }
  }

  template<typename T>
  void push_back_list_test(const char * message, int n)
  {
    std::cout << "==== " << message << "\n";

    reset();

    std::vector<std::list<T>> x;

    for (int i = 0 ; i < n; ++i)
    {
      x.push_back(std::list{T(i)});
      print_and_reset();
      std::cout << "size: " << x.size() << ", capacity: " << x.capacity() << "\n";
    }
  }

  void fill_test()
  {
    std::cout << "==== fill test\n";

    std::vector<int> x(0);

    double waste = 0;

    int n = 1024;
    //int n = 1065;

    for (int i = 0 ; i < n; ++i)
    {
      x.push_back(i);
      double this_waste = double(x.capacity() - x.size()) / double(x.size());
      //std::cout << "waste: " << this_waste << "\n";
      //std::cout << "size: " << x.size() << ", capacity: " << x.capacity() << "\n";
      waste += this_waste;
    }

    std::cout << "average waste: " << (waste / double(n)) << "\n";
  }
}

int main()
{
  push_back_test<instrumented_simple>("test for allocation steps", 40);

  int n = 6;
  push_back_test<instrumented_simple>("simple type", n);
  push_back_test<instrumented_container>("simple container", n);
  push_back_test<instrumented_sentinel>("container with dynamically allocated sentinel", n);
  push_back_test<instrumented_copy_only>("copy-only type", n);
  push_back_list_test<instrumented_simple>("list of simple type", n);

  fill_test();
  std::cout << "Done!\n";
}