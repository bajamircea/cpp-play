// compile with /std:c++20 /fsanitize=address

#include <cassert>
#include <coroutine>
#include <exception>
#include <memory>

// Uncomment one of the examples:
// EXAMPLE 1 is a repro in await_suspend returning a coroutine_handle
#define EXAMPLE 1
// EXAMPLE 2 is a repro in await_suspend of final_suspend returning a coroutine_handle
//#define EXAMPLE 2
// EXAMPLE 3 shows that await_suspend returning bool behaves correctly
//#define EXAMPLE 3

#if EXAMPLE == 1

struct [[nodiscard]] task
{
    struct promise_type
    {
    public:
        promise_type() noexcept = default;

        promise_type(const promise_type&) = delete;
        promise_type& operator=(const promise_type&) = delete;

        task get_return_object() noexcept
        {
            return { std::coroutine_handle<promise_type>::from_promise(*this) };
        }

        void return_void() noexcept
        {
        }

        void unhandled_exception() noexcept
        {
            std::terminate();
        }

        std::suspend_always initial_suspend() noexcept
        {
            return {};
        }

        std::suspend_always final_suspend() noexcept
        {
            return {};
        }
    };

    std::coroutine_handle<promise_type> m_child_coro{};

    task(std::coroutine_handle<promise_type> child_coro) noexcept :
        m_child_coro{ child_coro }
    {
    }
    ~task()
    {
        if (m_child_coro)
        {
            m_child_coro.destroy();
        }
    }

    task(const task&) = delete;
    task& operator=(const task&) = delete;
    task(task&& other) noexcept :
        m_child_coro{ other.m_child_coro }
    {
        other.m_child_coro = {};
    }

    void start() noexcept
    {
        m_child_coro.resume();
    }
};

struct awaiter_destroy_in_await_suspend
{
    std::unique_ptr<task>* m_continuation_step{ nullptr };

    constexpr bool await_ready() const noexcept
    {
        return false;
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept
    {
        m_continuation_step->reset();
        // This continues to use the coroutine frame that we just deallocated, instead of using the stack.
        // An assembly instruction like "mov rdx,qword ptr [rsp+0D8h]" incorrectly retrieves from the stack
        // an address pointing to (now deallocated) coroutine frame.
        return std::noop_coroutine();
    }
    void await_resume() const noexcept
    {
        std::terminate();
    }
};

task async_coro(std::unique_ptr<task>& continuation_step_)
{
    co_await awaiter_destroy_in_await_suspend(&continuation_step_);
    co_return;
}

int main()
{
    std::unique_ptr<task> allocated_task;
    allocated_task = std::make_unique<task>(async_coro(allocated_task));
    allocated_task->start();
    return 0;
}

#elif EXAMPLE == 2

struct [[nodiscard]] task
{
    struct promise_type
    {
        std::unique_ptr<task>* m_continuation_step{ nullptr };
    public:
        promise_type() noexcept = default;

        promise_type(const promise_type&) = delete;
        promise_type& operator=(const promise_type&) = delete;

        task get_return_object() noexcept
        {
            return { std::coroutine_handle<promise_type>::from_promise(*this) };
        }

        void return_void() noexcept
        {
        }

        void unhandled_exception() noexcept
        {
            std::terminate();
        }

        std::suspend_always initial_suspend() noexcept
        {
            return {};
        }

        struct final_awaiter
        {
            std::unique_ptr<task>* m_continuation_step{ nullptr };

            [[nodiscard]] constexpr bool await_ready() const noexcept
            {
                return false;
            }

            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type>) noexcept
            {
                assert(nullptr != m_continuation_step);
                m_continuation_step->reset();
                // Another example that continues to use the coroutine frame that we just deallocated, instead of using the stack.
                return std::noop_coroutine();
            }

            [[noreturn]] void await_resume() const noexcept
            {
                std::terminate();
            }
        };

        final_awaiter final_suspend() noexcept
        {
            assert(m_continuation_step != nullptr);
            return { m_continuation_step };
        }
    };

    std::coroutine_handle<promise_type> m_child_coro{};

    task(std::coroutine_handle<promise_type> child_coro) noexcept :
        m_child_coro{ child_coro }
    {
    }
    ~task()
    {
        if (m_child_coro)
        {
            m_child_coro.destroy();
        }
    }

    task(const task&) = delete;
    task& operator=(const task&) = delete;
    task(task&& other) noexcept :
        m_child_coro{ other.m_child_coro }
    {
        other.m_child_coro = {};
    }

    void start(std::unique_ptr<task>& continuation_step) noexcept
    {
        m_child_coro.promise().m_continuation_step = &continuation_step;
        m_child_coro.resume();
    }
};

task async_do_nothing()
{
    co_return;
}

int main()
{
    std::unique_ptr<task> allocated_task;
    allocated_task = std::make_unique<task>(async_do_nothing());
    allocated_task->start(allocated_task);
    return 0;
}

#elif EXAMPLE == 3

struct [[nodiscard]] task
{
    struct promise_type
    {
    public:
        promise_type() noexcept = default;

        promise_type(const promise_type&) = delete;
        promise_type& operator=(const promise_type&) = delete;

        task get_return_object() noexcept
        {
            return { std::coroutine_handle<promise_type>::from_promise(*this) };
        }

        void return_void() noexcept
        {
        }

        void unhandled_exception() noexcept
        {
            std::terminate();
        }

        std::suspend_always initial_suspend() noexcept
        {
            return {};
        }

        std::suspend_always final_suspend() noexcept
        {
            return {};
        }
    };

    std::coroutine_handle<promise_type> m_child_coro{};

    task(std::coroutine_handle<promise_type> child_coro) noexcept :
        m_child_coro{ child_coro }
    {
    }
    ~task()
    {
        if (m_child_coro)
        {
            m_child_coro.destroy();
        }
    }

    task(const task&) = delete;
    task& operator=(const task&) = delete;
    task(task&& other) noexcept :
        m_child_coro{ other.m_child_coro }
    {
        other.m_child_coro = {};
    }

    void start() noexcept
    {
        m_child_coro.resume();
    }
};

struct awaiter_destroy_in_await_suspend
{
    std::unique_ptr<task>* m_continuation_step{ nullptr };

    constexpr bool await_ready() const noexcept
    {
        return false;
    }

    bool await_suspend(std::coroutine_handle<>) noexcept
    {
        m_continuation_step->reset();
        // Here the local function variable `suspend` is correctly stored on the stack: no problem here.
        // I suspect the problem is linked to symmetric transfer method where await_suspend returs a coroutine_handle
        bool suspend = true;
        return suspend;
    }
    void await_resume() const noexcept
    {
        std::terminate();
    }
};

task async_coro(std::unique_ptr<task>& continuation_step_)
{
    co_await awaiter_destroy_in_await_suspend(&continuation_step_);
    co_return;// /std:c++20 /fsanitize=address
}

int main()
{
    std::unique_ptr<task> allocated_task;
    allocated_task = std::make_unique<task>(async_coro(allocated_task));
    allocated_task->start();
    return 0;
}

#endif