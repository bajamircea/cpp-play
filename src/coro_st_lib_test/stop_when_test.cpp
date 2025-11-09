#include <cassert>
#include <cstddef>
#include <coroutine>
#include <exception>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace coro_st
{
  class callback
  {
    using pure_callback_fn = void (*)(void* x) noexcept;

    void* x_{ nullptr };
    pure_callback_fn fn_ { nullptr };

  public:
    callback() noexcept = default;
    callback(void* x, pure_callback_fn fn) noexcept :
      x_{ x },
      fn_{ fn }
    {
      assert(nullptr != fn_);
    }

    callback(const callback&) noexcept = default;
    callback& operator=(const callback&) noexcept = default;

    void invoke() noexcept
    {
      assert(nullptr != fn_);
      fn_(x_);
    }

    void operator()() noexcept
    {
      invoke();
    }

    bool is_callable() noexcept
    {
      return nullptr != fn_;
    }
  };

  template<typename T, void (*fn)(T&) noexcept>
  struct make_function_callback_impl
  {
    static void invoke(void* x_void) noexcept
    {
      assert(x_void != nullptr);
      T* x = reinterpret_cast<T*>(x_void);
      return std::invoke(fn, *x);
    }
  };

  template<auto FnPtr, typename T>
  callback make_function_callback(T& x)
  {
    return callback{ &x, &make_function_callback_impl<T, FnPtr>::invoke };
  }

  template<typename T, void (T::*member_fn)() noexcept>
  struct make_member_callback_impl
  {
    static void invoke(void* x_void) noexcept
    {
      assert(x_void != nullptr);
      T* x = reinterpret_cast<T*>(x_void);
      return std::invoke(member_fn, x);
    }
  };

  template<auto MemberFnPtr, typename T>
  callback make_member_callback(T* x)
  {
    return callback{ x, &make_member_callback_impl<T, MemberFnPtr>::invoke };
  }

  inline callback make_resume_coroutine_callback(std::coroutine_handle<> handle)
  {
    return callback{ handle.address(), +[](void* x) noexcept {
      std::coroutine_handle<> original_handle = std::coroutine_handle<>::from_address(x);
      original_handle.resume();
    }};
  }
}

namespace cpp_util
{
  // Intrusive list
  //
  // Note: useful to use by deleting a node based on a reference to that node
  // - intrusive
  // - non-ownning
  // - double linked
  // - linear
  // - end iterator is nullptr
  // - header points to head and tail
  // - no dummy node
  // - no cached size
  template<typename Node, Node* Node::*next, Node* Node::*prev>
  class intrusive_list
  {
    Node* head_{ nullptr };
    Node* tail_{ nullptr };
  public:
    intrusive_list() noexcept = default;

    // Not owning, therefore copy does not copy nodes.
    // Delete copy to avoid mistakes where head and tail get updated
    // for a copy out of sync with the original
    intrusive_list(const intrusive_list&) = delete;
    intrusive_list& operator=(const intrusive_list&) = delete;

    intrusive_list(intrusive_list&& other) noexcept :
      head_{ other.head_ }, tail_{ other.tail_ }
    {
      other.head_ = nullptr;
      other.tail_ = nullptr;
    }

    intrusive_list& operator=(intrusive_list&& other) noexcept
    {
      head_ = other.head_;
      tail_ = other.tail_;
      other.head_ = nullptr;
      other.tail_ = nullptr;
    }

    bool empty() const noexcept
    {
      return head_ == nullptr;
    }

    void push_back(Node* what) noexcept
    {
      what->*next = nullptr;
      what->*prev = tail_;
      if (nullptr == tail_)
      {
        head_ = what;
      }
      else
      {
        tail_->*next = what;
      }
      tail_ = what;
    }

    void remove(Node* what) noexcept
    {
      if (what == head_)
      {
        head_ = what->*next;
      }
      else
      {
        what->*prev->*next = what->*next;
      }
      if (what == tail_)
      {
        tail_ = what->*prev;
      }
      else
      {
        what->*next->*prev = what->*prev;
      }
    }

    Node* front() noexcept
    {
      return head_;
    }

    Node* back() noexcept
    {
      return tail_;
    }

    Node* pop_front() noexcept
    {
      if (head_ == nullptr)
      {
        return nullptr;
      }
      Node* what = head_;
      head_ = what->*next;
      if (what == tail_)
      {
        tail_ = nullptr;
      }
      else
      {
        what->*next->*prev = nullptr;
      }
      return what;
    }
  };
}

namespace coro_st
{
  struct stop_list_node
  {
    stop_list_node* next{ nullptr };
    stop_list_node* prev{ nullptr };
    callback cb{};

    stop_list_node() noexcept = default;

    stop_list_node(const stop_list_node&) = delete;
    stop_list_node& operator=(const stop_list_node&) = delete;
  };

  using stop_list = cpp_util::intrusive_list<stop_list_node, &stop_list_node::next, &stop_list_node::prev>;

  class stop_source;

  template <typename Fn>
  class stop_callback;

  class stop_token
  {
    friend stop_source;

    template <typename Fn>
    friend class stop_callback;

    stop_source* source_ = nullptr;

    explicit stop_token(stop_source* source) noexcept : source_{ source }
    {
    }

  public:
    stop_token(const stop_token&) noexcept = default;
    stop_token& operator=(const stop_token&) noexcept = default;

    bool stop_requested() const noexcept;
  };

  class stop_source
  {
    template <typename Fn>
    friend class stop_callback;

    bool stop_{ false };
    stop_list callbacks_{};
  public:
    stop_source() noexcept = default;

    stop_source(const stop_source&) = delete;
    stop_source& operator=(const stop_source&) = delete;

    bool stop_requested() const noexcept
    {
      return stop_;
    }

    bool request_stop() noexcept
    {
      if (stop_)
      {
        return false;
      }
      stop_ = true;
      while(true)
      {
        stop_list_node* node = callbacks_.pop_front();
        if (node == nullptr)
        {
          break;
        }
        callback copy_cb = node->cb;
        node->cb = callback{};
        copy_cb.invoke();
      }
      return true;
    }

    stop_token get_token() noexcept
    {
      return stop_token{ this };
    }
  };

  inline bool stop_token::stop_requested() const noexcept
  {
    assert(source_ != nullptr);
    return source_->stop_requested();
  }

  template<>
  class stop_callback<callback>
  {
    stop_source* source_ = nullptr;
    stop_list_node node_;
  public:
    stop_callback(stop_token token, callback fn) noexcept :
      source_{ token.source_ }
    {
      assert(source_ != nullptr);
      assert(fn.is_callable());
      if (source_->stop_requested())
      {
        fn();
      }
      else
      {
        node_.cb = fn;
        source_->callbacks_.push_back(&node_);
      }
    }

    stop_callback(const stop_callback&) = delete;
    stop_callback& operator=(const stop_callback&) = delete;

    ~stop_callback()
    {
      if (node_.cb.is_callable())
      {
        source_->callbacks_.remove(&node_);
      }
    }
  };

  template <typename Fn>
  class stop_callback
  {
    stop_source* source_ = nullptr;
    Fn fn_;
    stop_list_node node_;
  public:
    stop_callback(stop_token token, Fn&& fn) noexcept :
      source_{ token.source_ }, fn_{ std::move(fn) }
    {
      assert(source_ != nullptr);
      if (source_->stop_requested())
      {
        fn_();
      }
      else
      {
        node_.cb = make_member_callback<&stop_callback::invoke>(this);
        source_->callbacks_.push_back(&node_);
      }
    }

    stop_callback(const stop_callback&) = delete;
    stop_callback& operator=(const stop_callback&) = delete;

    ~stop_callback()
    {
      if (node_.cb.is_callable())
      {
        source_->callbacks_.remove(&node_);
      }
    }

  private:
    void invoke() noexcept
    {
      fn_();
    }
  };
}

namespace coro_st
{
  class context
  {
    stop_token token_;

  public:
    explicit context(stop_token token) noexcept :
      token_{ token }
    {
    }

    context(const context&) = delete;
    context& operator=(const context&) = delete;

    stop_token get_stop_token() noexcept
    {
      return token_;
    }
  };

  template<typename Awaiter>
  concept has_void_await_suspend = requires(Awaiter a, std::coroutine_handle<> h)
  {
    { a.await_suspend(h) } noexcept -> std::same_as<void>;
  };

  template<typename Awaiter>
  concept has_bool_await_suspend = requires(Awaiter a, std::coroutine_handle<> h)
  {
    { a.await_suspend(h) } noexcept -> std::convertible_to<bool>;
  };

  template<typename Awaiter>
  concept has_symmetric_await_suspend = requires(Awaiter a, std::coroutine_handle<> h)
  {
    { a.await_suspend(h) } noexcept -> std::convertible_to<std::coroutine_handle<>>;
  };

  template<typename T>
  concept is_co_awaiter = requires(T a)
    {
      { a.await_ready() } noexcept -> std::convertible_to<bool>;
      a.await_resume();
      { a.start_as_chain_root() } noexcept;
      { a.get_result_exception() } noexcept -> std::same_as<std::exception_ptr>;
    } && (
      has_void_await_suspend<T> ||
      has_bool_await_suspend<T> ||
      has_symmetric_await_suspend<T>) &&
    !std::is_move_constructible_v<T> &&
    !std::is_move_assignable_v<T>;

  template<is_co_awaiter T>
  using co_awaiter_result_t = decltype(
    std::declval<T>().await_resume());

  template<typename T>
  concept is_co_work = requires(T x, context ctx)
    {
      // removed noexcept, in some cases allocation is required
      { x.get_awaiter(ctx) } -> is_co_awaiter;
    } &&
    std::is_nothrow_move_constructible_v<T> &&
    std::is_nothrow_move_assignable_v<T>;

  template<is_co_work T>
  using co_work_awaiter_t = decltype(
    std::declval<T>().get_awaiter(std::declval<context&>()));

  template<is_co_work T>
  using co_work_result_t = co_awaiter_result_t<co_work_awaiter_t<T>>;

  template<typename T>
  concept is_co_task = requires(T x)
    {
      { x.get_work() } noexcept -> is_co_work;
    } &&
    !std::is_move_constructible_v<T> &&
    !std::is_move_assignable_v<T>;

  template<is_co_task T>
  using co_task_work_t = decltype(
    std::declval<T>().get_work());

  template<is_co_task T>
  using co_task_awaiter_t = co_work_awaiter_t<co_task_work_t<T>>;

  template<is_co_task T>
  using co_task_result_t = co_awaiter_result_t<co_task_awaiter_t<T>>;

  template<typename Promise>
  class [[nodiscard]] unique_coroutine_handle
  {
  public:
    using handle_type = std::coroutine_handle<Promise>;

  private:
    handle_type h_;

  public:
    unique_coroutine_handle() noexcept :
      h_{ nullptr }
    {
    }

    explicit unique_coroutine_handle(const handle_type& h) noexcept :
      h_{ h }
    {
    }

    explicit unique_coroutine_handle(handle_type&& h) noexcept :
      h_{ std::move(h) }
    {
    }

    template<typename Arg1, typename Arg2, typename ... Args>
      requires(std::is_nothrow_constructible_v<handle_type, Arg1, Arg2, Args...>)
    unique_coroutine_handle(Arg1 && arg1, Arg2 && arg2, Args && ... args) noexcept
      :
      h_{ std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Args>(args) ... }
    {
    }

    ~unique_coroutine_handle()
    {
      close_if_valid();
    }

    unique_coroutine_handle(const unique_coroutine_handle &) = delete;
    unique_coroutine_handle & operator=(const unique_coroutine_handle &) = delete;

    unique_coroutine_handle(unique_coroutine_handle && other) noexcept :
      h_{ std::move(other.h_) }
    {
      other.h_ = nullptr;
    }

    unique_coroutine_handle & operator=(unique_coroutine_handle && other) noexcept
    {
      handle_type tmp = std::move(other.h_);
      other.h_ = nullptr;
      close_if_valid();
      h_ = std::move(tmp);
      return *this;
    }

    handle_type get() const noexcept
    {
      return h_;
    }

    bool is_valid() const noexcept
    {
      return h_ != nullptr;
    }

  private:
    void close_if_valid() noexcept
    {
      if (is_valid())
      {
        h_.destroy();
      }
    }
  };

  class [[nodiscard]] co
  {
  public:
    class promise_type
    {
      friend co;

      std::exception_ptr exception_{};

      context* pctx_{ nullptr };
      std::coroutine_handle<> parent_coro_;

    public:
      promise_type() noexcept = default;

      promise_type(const promise_type&) = delete;
      promise_type& operator=(const promise_type&) = delete;

      co get_return_object() noexcept
      {
        return {std::coroutine_handle<promise_type>::from_promise(*this)};
      }

      std::suspend_always initial_suspend() noexcept
      {
        return {};
      }

      std::suspend_always final_suspend() noexcept
      {
        assert(pctx_ != nullptr);
        return {};
      }

      void return_void() noexcept
      {
      }

      void unhandled_exception() noexcept
      {
        assert(nullptr == exception_);
        exception_ = std::current_exception();
      }

      void get_result() const
      {
        if (exception_)
        {
          std::rethrow_exception(exception_);
        }
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        return exception_;
      }

    };

  private:
    class [[nodiscard]] awaiter
    {
      unique_coroutine_handle<promise_type> unique_child_coro_;

    public:
      awaiter(context& ctx, unique_coroutine_handle<promise_type>&& unique_child_coro) noexcept :
        unique_child_coro_{ std::move(unique_child_coro) }
      {
        unique_child_coro_.get().promise().pctx_ = &ctx;
      }

      awaiter(const awaiter&) = delete;
      awaiter& operator=(const awaiter&) = delete;

      [[nodiscard]] constexpr bool await_ready() const noexcept
      {
        return true;
      }

      void await_suspend(std::coroutine_handle<>) noexcept
      {
      }

      void await_resume()
      {
        return;
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        return {};
      }

      void start_as_chain_root() noexcept
      {
      }
    };

    class [[nodiscard]] work
    {
      unique_coroutine_handle<promise_type> unique_child_coro_;

    public:
      work(std::coroutine_handle<promise_type> child_coro) noexcept :
        unique_child_coro_{ child_coro }
      {
      }

      work(const work&) = delete;
      work& operator=(const work&) = delete;
      work(work&&) noexcept = default;
      work& operator=(work&&) noexcept = default;

      [[nodiscard]] awaiter get_awaiter(context& ctx) noexcept
      {
        return {ctx, std::move(unique_child_coro_)};
      }
    };

  private:
    work work_;

    co(std::coroutine_handle<promise_type> child_coro) noexcept :
      work_{ child_coro }
    {
    }

  public:
    co(const co&) = delete;
    co& operator=(const co&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return std::move(work_);
    }
  };

  class [[nodiscard]] suspend_forever_task
  {
    class [[nodiscard]] awaiter
    {
      context& ctx_;
      std::optional<stop_callback<callback>> parent_stop_cb_;

    public:
      explicit awaiter(context& ctx) noexcept :
        ctx_{ ctx }
      {
      }

      awaiter(const awaiter&) = delete;
      awaiter& operator=(const awaiter&) = delete;

      [[nodiscard]] constexpr bool await_ready() const noexcept
      {
        return false;
      }

      void await_suspend(std::coroutine_handle<>) noexcept
      {
        configure_cancellation();
      }

      constexpr void await_resume() const noexcept
      {
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        return {};
      }

      void start_as_chain_root() noexcept
      {
        configure_cancellation();
      }

    private:
      void configure_cancellation() noexcept
      {
        parent_stop_cb_.emplace(
          ctx_.get_stop_token(),
          make_member_callback<&awaiter::on_cancel>(this));
      }

      void on_cancel() noexcept
      {
        parent_stop_cb_.reset();
      }
    };

    struct [[nodiscard]] work
    {
      work() noexcept = default;

      work(const work&) = delete;
      work& operator=(const work&) = delete;
      work(work&&) noexcept = default;
      work& operator=(work&&) noexcept = default;

      [[nodiscard]] awaiter get_awaiter(context& ctx) noexcept
      {
        return awaiter{ ctx };
      }
    };

  public:
    suspend_forever_task() noexcept = default;

    suspend_forever_task(const suspend_forever_task&) = delete;
    suspend_forever_task& operator=(const suspend_forever_task&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return {};
    }
  };

  [[nodiscard]] inline suspend_forever_task async_suspend_forever() noexcept
  {
    return {};
  }

  template<is_co_task CoTask1, is_co_task CoTask2>
  class [[nodiscard]] stop_when_task
  {
    using CoWork1 = co_task_work_t<CoTask1>;
    using CoWork2 = co_task_work_t<CoTask2>;
    using CoAwaiter1 = co_task_awaiter_t<CoTask1>;
    using CoAwaiter2 = co_task_awaiter_t<CoTask2>;

    class [[nodiscard]] awaiter
    {
      context& parent_ctx_;
      std::coroutine_handle<> parent_handle_;
      std::optional<stop_callback<callback>> parent_stop_cb_;
      stop_source children_stop_source_;
      size_t pending_count_{ 0 };

      context task_ctx1_;
      CoAwaiter1 co_awaiter1_;

      context task_ctx2_;
      CoAwaiter2 co_awaiter2_;

    public:
      awaiter(
        context& parent_ctx,
        CoWork1& co_work1,
        CoWork2& co_work2
      ) :
        parent_ctx_{ parent_ctx },
        parent_handle_{},
        parent_stop_cb_{},
        children_stop_source_{},
        pending_count_{ 0 },
        task_ctx1_{ children_stop_source_.get_token() },
        co_awaiter1_{ co_work1.get_awaiter(task_ctx1_) },
        task_ctx2_{ children_stop_source_.get_token() },
        co_awaiter2_{ co_work2.get_awaiter(task_ctx2_) }
      {
      }

      awaiter(const awaiter&) = delete;
      awaiter& operator=(const awaiter&) = delete;

      [[nodiscard]] constexpr bool await_ready() const noexcept
      {
        return false;
      }

      void await_suspend(std::coroutine_handle<>) noexcept
      {
      }

      void await_resume()
      {
      }

      std::exception_ptr get_result_exception() const noexcept
      {
        return {};
      }

      void start_as_chain_root() noexcept
      {
        pending_count_ = 1;
        init_parent_cancellation_callback();

        start_chains();

        --pending_count_;
        if (0 != pending_count_)
        {
          return;
        }

        parent_stop_cb_.reset();
      }

    private:

      void init_parent_cancellation_callback() noexcept
      {
        parent_stop_cb_.emplace(
          parent_ctx_.get_stop_token(),
          make_member_callback<&awaiter::on_parent_cancel>(this));
      }

      void on_parent_cancel() noexcept
      {
        parent_stop_cb_.reset();
        children_stop_source_.request_stop();
      }

      void start_chains() noexcept
      {
        assert(1 == pending_count_);
        pending_count_ = 2;
        co_awaiter1_.start_as_chain_root();

        if (1 == pending_count_)
        {
          // co_awaiter_ completed early, no need to sleep
          return;
        }

        ++pending_count_;
        co_awaiter2_.start_as_chain_root();
      }
    };

    struct [[nodiscard]] work
    {
      CoWork1 co_work1_;
      CoWork2 co_work2_;

      work(
        CoTask1& co_task1,
        CoTask2& co_task2
      ) noexcept:
        co_work1_{ co_task1.get_work() },
        co_work2_{ co_task2.get_work() }
      {
      }

      work(const work&) = delete;
      work& operator=(const work&) = delete;
      work(work&&) noexcept = default;
      work& operator=(work&&) noexcept = default;

      [[nodiscard]] awaiter get_awaiter(context& ctx)
      {
        return {ctx, co_work1_, co_work2_};
      }
    };

  private:
    work work_;

  public:
    stop_when_task(
      CoTask1& co_task1,
      CoTask2& co_task2
    ) noexcept :
      work_{ co_task1, co_task2 }
    {
    }

    stop_when_task(const stop_when_task&) = delete;
    stop_when_task& operator=(const stop_when_task&) = delete;

    [[nodiscard]] work get_work() noexcept
    {
      return std::move(work_);
    }
  };

  template<is_co_task CoTask1, is_co_task CoTask2>
  [[nodiscard]] stop_when_task<CoTask1, CoTask2>
    async_stop_when(CoTask1 co_task1, CoTask2 co_task2)
  {
    return {co_task1, co_task2};
  }
}

namespace test
{
  using test_fn = void(*)();

  struct test_base
  {
    test_base(test_fn fn, const char * name, const char * file, int line);

    test_fn fn_;
    const char * name_;
    const char * file_;
    const int line_;
    test_base * next_;
  };

  void fail_current(const char * file, int line, const char * message);

  int run();
} // namespace test

#define TEST(func) void func(); test::test_base func ## _instance{func, #func, __FILE__, __LINE__}; void func()

#define FAIL_TEST(reason) test::fail_current(__FILE__, __LINE__, "FAILED_TEST(" #reason ")")

#define ASSERT_TRUE(cond) if (cond) {} else { test::fail_current(__FILE__, __LINE__, "ASSERT failed '" #cond "' was false"); }

#define ASSERT_FALSE(cond) if (cond) { test::fail_current(__FILE__, __LINE__, "ASSERT_FALSE failed '" #cond "' was true"); }

#define ASSERT_EQ(expected, actual) if (expected == actual) {} else { test::fail_current(__FILE__, __LINE__, "ASSERT_EQ failed for expected: '" #expected "' and actual '" #actual "'"); }

#define ASSERT_NE(expected, actual) if (expected == actual) { test::fail_current(__FILE__, __LINE__, "ASSERT_NE failed for expected: '" #expected "' and actual '" #actual "'"); }

#define ASSERT_THROW(expr, ex) try{ {expr;} test::fail_current(__FILE__, __LINE__, "ASSERT_THROW failed for expression '" #expr "'"); } catch (const ex &) {}

#define ASSERT_NO_THROW(expr) { bool assert_no_throw = true; try{ {expr;} assert_no_throw = false; } catch(...) {} if (assert_no_throw) { test::fail_current(__FILE__, __LINE__, "ASSERT_NO_THROW failed for expression '" #expr "'"); }

#define ASSERT_THROW_WHAT(expr, ex, what_str) try{ {expr;} test::fail_current(__FILE__, __LINE__, "ASSERT_THROW_WHAT failed for expression '" #expr "'"); } catch (const ex & e) { ASSERT_EQ(what_str, std::string(e.what())); }

namespace
{
  test::test_base * head_test = nullptr;
  test::test_base * crt_test = nullptr;

  bool crt_failed;

  struct test_exception
  {
  };

  void log_exception()
  {
    try
    {
      throw;
    }
    catch (const test_exception &)
    {
    }
    catch (const std::runtime_error & e)
    {
      std::cout << "  caught std::runtime_error: " << e.what() << "\n";
    }
    catch (const std::exception & e)
    {
      std::cout << "  caught std::exception: " << e.what() << "\n";
    }
    catch (...)
    {
      std::cout << "  caught unknown exception\n";
    }
  }

} // anonymous namespace

namespace test
{
  test_base::test_base(test_fn fn, const char * name, const char * file, int line) :
    fn_{ fn }, name_{ name }, file_{ file }, line_{ line }, next_{ head_test }
  {
    head_test = this;
  }

  void fail_current(const char * file, int line, const char * message)
  {
    crt_failed = true;
    std::cout
      << file << ":" << line
      << ":1: error: "
      << crt_test->name_ << " " << message << "\n";
    throw test_exception();
  }

  int run()
  {
    bool any_failed = false;

    for (crt_test = head_test;
        crt_test != nullptr;
        crt_test = crt_test->next_)
    {
      crt_failed = false;

      std::cout << "test "
        << crt_test->name_ << "\n";

      try
      {
        crt_test->fn_();
      }
      catch(...)
      {
        crt_failed = true;
        log_exception();
      }

      if (crt_failed)
      {
        any_failed = true;
        std::cout
          << crt_test->file_ << ":" << crt_test->line_
          << ":1: error: "
          << crt_test->name_ << " failed\n";
      }
    }

    if (any_failed)
    {
      std::cout << "[FAILED]\n";
    }
    else
    {
      std::cout << "[OK]\n";
    }

    return any_failed ? 1 : 0;
  }
} // namespace test

namespace
{
  template<coro_st::is_co_task CoTask>
  void run2(CoTask co_task)
  {
    coro_st::stop_source main_stop_source;

    coro_st::context ctx{ main_stop_source.get_token() };

    auto co_awaiter = co_task.get_work().get_awaiter(ctx);

    co_awaiter.start_as_chain_root();
  }

  // For some reason that triggers what I believe to be a false positive
  // on g++ that made me use -Wno-dangling-pointer on g++ -O3 build
  TEST(stop_when_exception2)
  {
    auto async_lambda = []() -> coro_st::co {
      co_return;
    };

    run2(coro_st::async_stop_when(
      coro_st::async_suspend_forever(),
      async_lambda()
    ));
  }

} // anonymous namespace

int main()
{
  return test::run();
}
