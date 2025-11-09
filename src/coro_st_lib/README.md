# Basic example

```cpp
  // async_some_wait is a coroutine returning an integer
  coro_st::co<int> async_some_wait()
  {
    // result is a std::tuple<void_result, int>
    auto result = co_await coro_st::async_wait_all(
      coro_st::async_sleep_for(std::chrono::seconds(0)),
      std::invoke([]() -> coro_st::co<int> {
        co_return 42;
      })
    );
    static_assert(std::is_same_v<coro_st::void_result&, decltype(std::get<0>(result))>);
    static_assert(std::is_same_v<int&, decltype(std::get<1>(result))>);
    co_return std::get<1>(result);
  }

  TEST(wait_all_inside_co)
  {
    // run returns an std::optional of the return type of async_some_wait()
    // if async_some_wait() is stopped before it completes then
    //   the std::optional is nullopt
    // if async_some_wait() throws then run rethrows that exception
    auto result = coro_st::run(async_some_wait()).value();
    ASSERT_EQ(42, result);
  }
```


# What is this?

Purely single threaded coroutine framework. Pure single threadness is quite
a limitation to actual real usage. But it's useful:
- as a learning tool (coroutine library code is complicated enough without
  adding multithreading to the mix)
- as a benchmark against which threading support can be measured

The threading model is:
- single threaded (obviously)
- on that thead call `run` with a root entry point
- `run` will internally start the entry point, then loop piking
  alternativly from a `ready_queue` of work to be done and a `timer_heap`
  of timers, sleeping when there is nothing to be done yet.
- `run` returns (or throws) when the root entry point eventually
  completes/stops


# Can be extended outside a pure single threaded case?

Yes, it can be extended to "largely single threaded" reasonably easy by:
- copy pasting and
  - changing code around the `ready_queue` to allow work to be inserted
    from another thread
  - changing code in `run` to accept a thread safe `stop_token` type
    which inserts a "stop work/boolean" from another thread
- or adding a layer of templatizing on top of the `context`

It can be extended to multithreading by carefully revisting all code of single
threaded, it will require a lot of synchronization and would be more complex
including at usage time e.g. when forking via e.g. `async_wait_all` then the child
coroutines might run on different threads and require synchronization on any data
they share (identifying that data is time consumming for the programmer and error
prone).


# Design choices

**We want to have structured concurrency** with (relatively) simple reasoning
rules about work lifetimes.

In particular **avoid detached work** where functions return before the work is
completed (i.e. on another thread, another coroutine etc.). Detached work has
many problems including:
- requires joining later (which is problematic for coroutines where that
  can't be done currently in a destructor)
- the work might reference objects that go out of scope before the work is
  completed (from as simple as the calling function exits, to subtler cases
  of lambda captures by reference)

There is a coroutine type. Often this is called `task`, but taking
inspiration from a Google talk on coroutine adoption, will call it `co`, for
brevity. `co` is templatized on the return value type e.g. `co<std::string>`.

Structured concurrencly means e.g. that when we reach a `co_await` in our library
the code will suspend and continue only when the work of what's in the right of `co_await` completed, either with a result value or with an exception or
cancelled. This creates a chain of parent/child relationships, where the parent
is the continuation of the child. This is the most basic of the concurrency
primitives.

We'll use the convention that things that are made to be `co_await`ed are
prefixed with `async_`. This helps mitigate the case where we call the ramp of
a coroutine and we don't `co_await` it, therefore it does not do the work.

In additonal to the linear chains we want at least three concurrency primitives
that fan out and create multiple chains and handle the joining:
- `async_wait_any`
  - fans out the children provided, starts a number of chains (one per child)
  - when one of them completes (even though exception): cancel the rest
  - return the result of the first only when all finish (either straight or
    cancelled)
- `async_wait_all`
  - fans out the children provided, starts a number of chains (one per child)
  - return when all complete
- `async_with_nursery`
  - this is a low level concurency primitive (more details later)

Clearly `async_wait_any` requires cancellation, but actually `async_wait_all`
requires cancellation as well. When one of the chains of `async_wait_all` throws,
then the remaining will be cancelled.

Cancellation support is thus baked in thoughout.


# Implementing a `co_await`able function that is not a coroutine

Coroutine are easy to create, but require a nominal heap allocation.
Things like `async_wait_any` are implemented low level, they
are not a coroutine, they can avoid allocations. There is an
advantage for the leaves of coroutine chains to not require
allocations (especially if called in a loop). But they are
harder to code.

This framework uses a pattern where things that can be `co_await`ed are a `is_co_task` concept. Many of the methods in the implementation are `noexcept`.

- `is_co_task` concept for a type:
  - which cannot be moved
  - has a method `get_work() noexcept` returning a `is_co_work`
- `is_co_work` concept for a type:
  - which can be moved
  - has a method `get_awaiter(context&)` returning a `is_co_awaiter`
    - `get_awaiter` is NOT noexcept
- `is_co_awaiter` concept for a type:
  - has `await_ready`, `await_suspend`, `await_resume`
    for when `co_await`ed in a coroutine
  - and the additional
    - `start_as_chain_root() noexcept` for when trampolined from outside a coroutine
    - `get_result_exception() noexcept` to check e.g. in `async_wait_all` if a child
      had an error (exception means error)


# Code description

All the code is in the `coro_st` namespace. `st` stands for "(purely) single threaded".

If you want to just start playing with it, include the `coro_st.h` header,
see the very basic examples in `coro_st_lib_test\run_test.cpp`.

Here is a description in logical order of the contents library. For usage see the
associated test folder:
- `synthetic_coroutine.h`
  - NOT USED: see below why
  - demonstrates a technique where you could have a coroutine frame on the stack
  - `coroutine_frame_abi` is the ABI where the coroutine frame has two function pointers
  - `synthetic_resumable_coroutine_frame`
    - demonstrates how you could have a coroutine frame on the stack
    - to which you can get a `std::coroutine_handle<>`
    - which when it has `resume()` called, it calls your callback
  - the problem with this approach is that it's frail
    - the ABI is not standardised and subject to change
    - any other call on the returned coroutine handle is problematic e.g.
      - `destroy()` will hopefully terminate
      - `done()` is undefined behaviour
- `callback.h`
  - rationale: we end up using this a lot instead of `synthetic_coroutine` as
    a unified abstraction between coroutine resumption and other completion
    work.
    - it provides a "pure callback" to "do something (on an object)"
    - utilities to create such "pure callback" doing the ugly casts to/from `void*`
  - `callback`
    - the "pure callback"
    - captures a pointer
    - and a `void (*)(void* x) noexcept` function that takes that pointer
  - `make_function_callback`
    - helper function make a `callback` calling a `void (*)(T&) noexcept`
  - `make_member_callback`
    - helper function make a `callback` calling a member function for some
      class (i.e. `void (T::*member_fn)() noexcept`)
  - `make_resume_coroutine_callback`
    - helper function to make a `callback` that resumes a coroutine handle
- `stop_util.h`
  - single threaded implementation of the standard variants
    - however compared with `std::stop_source` has better `noexcept`
      guarantees e.g. does not use a heap allocated state for `stop_source`
    - largely similar to the `std::inplace` variants added in C++26
  - `stop_source` has a boolean that can be flipped to `true` using
    `request_stop()`
  - `stop_token` is really a pointer to `stop_source` to allow checking the
    state using `stop_requested()`
  - `stop_callback` calls a function object when the request to stop is called
    for the first time or when constructed. It uses a linked list provided
    by the `stop_source`
    - `std::optional<stop_callback<callback>>` is used a lot (e.g. due to size
       known before constructing)
- `ready_queue.h`
  - `ready_queue` is an intrusive queue of ready work
    - by the time work got there it's too late to not do it
  - `ready_node` the node in the queue contains:
    - `next`the pointer required for the queue
    - the work to be done as pure `callback`
- `timer_heap.h`
  - `timer_heap` is an intrusive heap of timers
  - `timer_node` the node in the heap contains:
    - pointers for `parent`, `left` and `right`
    - `deadline` for ordering by a monotonic `steady_clock` absolute time
    - the work to be done as pure `callback`
  - the rationale is:
    - we want an intrusive structure where the nodes can be stored
      in coroutine awaiters
    - the heap is a tree structure, it has the advantage over e.g.
      a red-black tree that that the next timer to use for sleep
      calculation is at the root, accessible in `O(1)`
    - we want reasonable time complexity for the other operations,
      we get `O(lg(N))` for insertion and removal, but the cost is not
      trivial as there might be lots of pointers to adjust in nodes
      for these operations
    - often heaps are implemented on top of memory contiguous data
      structures like `std::vector` where the node pointers are not
      needed and the index arithmetic is used to find `parent`, `left`
      and `right` values; the node based implementation has the
      advantage that nodes can be removed in `O(lg(N))` (e.g. when a timer
      in cancelled) and that insertion in particular can be made `noexcept`
      which is useful to be able to use a timer in error recovery
      scenarios: e.g. on exception sleep for a while and then try again,
      that would be problematic if sleeping could throw
- `event_loop_context.h`
  - `event_loop_context` holds references to the ready queue and heap and
    allows:
    - adding node to ready queue
    - adding node to the timer heap
    - removing node from timer heap (e.g. when timer cancelled)
  - this is somehow similar to a scheduler in the sender/receiver
    framework
- `chain_context.h`
  - `chain_context`
    - holds data for a cancellable chain:
      - the cancellation token
      - the continutation callback to run on completion of the chain
      - the cancellation callback to run when the chain was cancelled
      - a node that can be used to schedule callbacks
    - `run`, `async_wait_any` etc. use the `chain_context` to customize
       what should happen on child completion:
      - `run` to decide to exit the running loop
      - `async_wait_any`
         - to cancel the other children when the first completes
         - to determine when the last child completed and it should continue
           the parent
    - this is somehow similar to `set_[value|error|stopped]` in the
      sender/receiver framework
- `context.h`
  - `context`
    - holds references to:
      - an `event_loop_context`
      - a `chain_context`
    - except for the root context e.g. in `run`, the rest are created per chain
      by using the `event_loop_context` reference from the parent
      and a new `chain_context` (via a constructor)
- `event_loop.h`
  - `event_loop`
    - helper class holding a `ready_queue` and a `timer_heap`
    - `do_current_pending_work`
      - reads current pending tasks from both queue and heap and runs them
      - returns a duration to sleep if there is no more ready work, but
        there are pending timers
      - it's supposed to have some fairness e.g.
       - we consume alternatively from both the `ready_queue` and the `timer_heap`
       - from `ready_queue` we consume only what's present on arrival, invoking
         work might add more (which will be dealt with on a later iteration)
       - from the `timer_heap` we consume one by one and only to a captured `now`
- `coro_type_traits.h`
  - concepts and type deduction
  - `is_co_task`, `is_co_work`, `is_co_awaiter` concepts that can be used to enforce
    template arguments
  - `co_task_result_t` etc. gets the type of the result; i.e. you have a task,
    which gives a work, which gives an awaiter, which has a `await_resume`
    which returns a type: that type
- `void_result.h`
  - `struct void_result{};` to use in place of `void`
    - because can't have a `std::optional<void>`,
      but can have `std::optional<void_result>`
- `value_type_traits.h`
  - `value_type_t` normally maps a type to itself, but `void` to `void_result`
    - e.g. if a type T can be void use
      `std::optional<value_type_traits::value_type_t<T>>`
- `run.h`
  - `run(CoTask co_task)`
    - a function that start the task (e.g. a coroutine), waits until it completes
      returns an optional of the task return type (`void_result` instead of `void`)
      - the optional is `nullopt` if the task is cancelled
      - `run` throws if the task throws
    - like sender/receiver `sync_wait`, but runs the ready queue and
      timer heap,
- `unique_coroutine_handle`
  - a RAII type owning a coroutine handle
- `promise_base`
  - base for coroutine promises to handle variations around the coroutine
    return value
  - template on a type `T`, specialized for `void`
  - handles `return_value`/`return_void` and `unhandled_exception`
  - provides `T get_result()` to either get the value or throw the exception
- `co.h`
  - `co` key points:
    - is the (declared) return type of a coroutine
      - e.g. `co<int> async_foo(){ co_return 42; }`
    - templated on the type `co_return`ed from the coroutine
    - manages lazy coroutine e.g. it sets the parent continuation in the child promise
      before the child coroutine is first resumed/started
    - the child coroutine never runs longer than the parent
    - NOTE: coroutine means a (nominal) allocation: the compiler might optimize it out,
      but (performance wise) the assumption should be that we pay for a heap allocation
      for the coroutine frame, `co` does not provide the option of a custom allocator
    - `co` (via it's work, awaiter and unique_coroutine_handle) owns and destroys the
      coroutine frame
    - `await_transform`
      - takes a `is_co_task` by value, this enforces safety e.g. of references as
        as arguments, at the cost that `async_foo` ramp has to be called at a `co_await`
        point
      - via `co_task.get_work().get_awaiter(*pctx_)` it injects the context into child
        tasks: this allows cancellation, continuation and scheduling of continuation
      - the cost for this is an extra pointer to the context per coroutine frame
    - continuation, cancellation, resumption of parent are carefully orchestrated:
      either invoked immediately or scheduled for later or via `await_suspend` return
      values. The goal is to avoiding stack overflow of he sort that asymmetric transfer was meant to avoid. This is also done for the other `async_...` primitives.
- `yield.h`
  - `co_await async_yield();`
    - like std::this_thread::yield() than co_yield
    - the framework is nonpreemptive, you might want to let other chains to also make
      some progress
    - allows other chains to run by scheduling itself at the end of the ready queue
    - does not throw, does not heap allocate
- `sleep.h`
  - `co_await async_sleep_for(10s);`
    - sleeps, leaving other chains to work in meantime
    - uses the timer heap
    - does not throw, does not heap allocate: hence can be used at recovery points
      where you catch all exceptions, sleep, try again
- `suspend_forever.h`
  - `co_await async_suspend_forever();`
    - nothing is forever: it's until stopped via cancellation
      - but `async_suspend_until_stopped_via_cancellation` is too long
    - useful e.g. to keep a `std::jthread` variable alive until stopped.
      When `co_await`ed things are cancelled they behave as if an invisible
      exception is thrown: i.e. the coroutine frames are destructed, variables
      are destructed in the reverse order as they appear. E.g. the `std::jthread`
      variable will stop (if correctly coded) and join the thread it controlls.
- `noop.h`
  - `co_await async_noop();`
    - check for cancellation
    - if not cancelled it does not even suspend
    - useful to allow cancellation in long running activities
- `wait_any_two.h`
  - `co_await async_wait_any_two(task1, task2)`
    - NOT USED: written as a learning tool,
      use `async_wait_any`, there is no good reason not to
    - version of `async_wait_any` with simpler code avoding complexity for just two tasks
      - no variadic arguments
      - no need for tuple
      - requires return type is the same for both tasks
      - on completion, it does not indicate which of the two tasks completed (misses the index)
- `wait_any_type_traits.h`
  - helper for the return type of `co_await async_wait_any(...)` (see below)
- `wait_any.h`
  - `co_await async_wait_any(...)`
    - fans out the children provided, starts a number of chains (one per child)
    - when the first one of them completes (even though error.exception):
      cancel the rest
    - return the result of the first only when all finish (either straight or
      cancelled)
    - in `auto result = co_await async_wait_any(...)` result is a struct
      - the struct has an `index` member that gives the zero based index
        of the child that was te first to complete
      - if all the children return `void`, than the struct only has the index
      - else if some children do not return `void` then the struct also has a `value`
        - the type of the value is the common type excluding `void`
          - e.g. if one child returns `int`, the other `void` and the third `short`
            then the struct of has and `size_t index` and an `int value` as member
        - if there is no common type, you need to arrange a common type e.g. by
          returning a `std::variant` of the individual types
    - if the parent is cancelled, `async_wait_any` cancels all it's children
    - if a first completing child initiates a cancellation/stop then the 
      cancellation is propagated to the parent
- `wait_all.h`
  - `co_await async_wait_all(...)`
    - fans out the children provided, starts a number of chains (one per child)
    - if one of them completes though error/exception or cancel/stop: cancel the rest
    - in `auto result = co_await async_wait_all(...)` result is a tuple of
      each result type of each child, using `void_result` as a substitute for
      `void`
    - if the parent is cancelled, `async_wait_all` cancels all it's children
    - if a child initiates a cancellation/stop then the cancellation
      is propagated to the parent
- `wait_for.h`
  - `co_await async_wait_for(task, duration e.g. 1ms)`
    - can be applied to any task to stop it when a timeout is reached
    - called `timeout` in some libraries
    - returns an optional of the task return type (`void_result` instead of `void`)
      - the optional is `nullopt` if the task is cancelled due to duration
    - same functionality can be achieved with
      `co_await async_wait_any(task, async_sleep(duration))` except than:
      - `async_wait_for` optional return is easier to use/check than
        the index approach of of `async_wait_any`
      - `async_wait_for` is more optimal in terms of resources it needs
        to track progress
        - including that it does not start the timer if the provided task
          completes immediately
- `stop_when.h`
  - `co_await async_stop_when(task1, task2)`
    - run task1 and task2, cancel the other when the first completes
    - returns a `std::optional` for the return type of task1
      - if task2 completes or stops first return a `nullopt`
      - if task1 stops instead of completing then stop is propagated to the parent
    - called `take_until` elsewhere
    - name from appendix of P2175r0/libunifex
    - e.g. `co_await async_stop_when(task, async_sleep(duration))` is
      equivalent to `co_await async_wait_for(task, duration)`
    - but `async_stop_when` is a generalization of `async_wait_for`
      for when we want to stop task1 for other reasons than time elapesed
      e.g. stop when CTRL-C was pressed
      `co_await async_stop_when(task, async_ctrl_c_pressed())`
    - compared with `async_wait_any` it has similar benefits in usage and resources
      as `async_wait_for`
- `call_capture.h`
  - `call_capture` used by the nursery to capture a function and the arguments
    for `spawn_child` so that they can be kept alive until the (child) coroutine
    completes
    - very similar to what `std::bind` does
    - uses `std::decay` so that a `const &` argument is captured by value
    - does not work with rvalue arguments (similar to `std::bind`) at least for now
- `nursery.h`
  - `nursery`
    - allows more complex fanning out than `wait_[any|all]` with
      some additional runtime costs and care required
    - "nursery" = "where the children live"
    - to use:
      - instantiate a `nursery n;`
      - `co_await n.async_run(intial task)`
      - pass the nursery by reference to the inital task (as
        argument or lambda capture)
      - from the initial task or other children start further
        children with `n.spawn_child(...)`
        - `spawn_child` uses a heap allocation to store the child
          info for the lifetime of the child 
        - the syntax is a bit weird on the style of `std::bind`
          rather than a normal call as in normal `co_await`s
        - use `std::ref` to avoid accidental copy of arguments
        - you need to ensure that args passed by reference live
          long enough, or else pass them by value
      - `n.async_run` completes when the last child completes
        - you can cancel all children from one of them using
          `n.request_stop()`
      - if one child throws `n.async_run` cancels the rest and
        throws
      - `co_await n.async_run(...)` returns void
      - to actually return something you need to pass to the
        relevan children a reference to a value preceeding `n.async_run(...)`
- `event.h`
  - `event`
    - create one, pass it to several chains
    - some chains `co_await e.async_wait()`
    - some (usually one) calls:
      - `e.notify_one()` to schedule one to resume
        - returns `true` if one was scheduled, `false` if none was
      - `e.notify_all()` to schedule all waiting to resume
- `just_stopped.h`
  - `co_await async_just_stopped()`
    - when you have a tree of fanned out chains you can trigger cancellation
      from the base (to propagate to the leaves) OR from a leaf (to propagate
      to all other leaves and then to the root)
    - triggers a leaf cancellation
    - this might hit the base chain
      - e.g. the one in `run` which would then return a `nullopt`
      - use `stopped_as_optional` to stop earlier
- `stopped_as_optional.h`
  - `co_await async_stopped_as_optional(task)`
    - stops a leaf cancellation and transforms it into a `nullopt`
    - else it's the value of the task
- `just.h`
  - `auto result = co_await async_just(value)`
    - result = value
    - in a coroutine it's simpler to just assign a variable or `co_return` a value
    - useful for testing with tasks that immediately produce a value
      - e.g. a `async_read` from a socket might suspend while the first
        IP packet arrives over the network, but only return part of that
        packet and then a subsequent invocation might immediately produce
        a value from the unconsumed data in the previous packet, so many
        primitives have to deal with immediately produced values (e.g. in
        `start_as_chain_root`)
- `just_exception.h`
  - `auto result = co_await async_just_exception(std::runtime_error("Ups!"))`
    - throws the exception
    - in a coroutine it's simpler to just throw
      - but be mindful of throwing, as exceptions are propagated, but it's very
        expensive
    - useful for testing with tasks that immediately produce an exception/error
