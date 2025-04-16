# What is this?

Single threaded coroutine framework. It's useful as a learning tool
and as a benchmark against which threading support can be measured,
but the single threadness is quite a limitation to actual real usage.

The threading model is (single threaded obviously):
- on a thead call `run` with a root entry point
- `run` will internally start the entry point, then loop piking
  alternativly from a `ready_queue` of work to be done and a `timer_heap`
  of timers, sleeping when there is nothing to be done yet.
- `run` returns (or throws) when the root entry point eventually
  completes


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

Obviously there is a coroutine type. For brevity will call it `co`, templatized
on the return value e.g. `co<std::string>`. Structured concurrencly means e.g.
that when we reach a `co_await` in our library the code will suspend and
continue only when the work of what's in the right of `co_await` completed,
either with a result value or with an exception.
This creates a chain of parent/child relationships, where the parent is the
continuation of the child. This is the most basic of the concurrency primitives.

We'll use the convention that things that are made to be `co_await`ed are
prefixed with `async_`. This helps mitigate the case where we call the ramp of
a coroutine and we don't `co_await` it, therefore it does not do the work.

In additonal to the linear chains we want at least three concurrency primitives
that fan out and create multiple chains and handle the joining:
- `async_wait_any`
  - fans out, starts a number of chains
  - when one of them completes (even though exception): cancel the rest
  - return the result of the first
- `async_wait_all`
  - fans out, starts a number of chains
  - return the all of them complete
- `async_with_nursery`
  - this is a low level concurency primitive (more details later)

Clearly `async_wait_any` requires cancellation, but actually `async_wait_all`
requires cancellation when one of the chains throws then the remaining will
be cancelled.


# Code description

All the code is in the `coro_st` namespace.

If you want to just start playing with it, include the `coro_st.h` header,
see the examples in `coro_st_lib_test\run_test.cpp`

- `synthetic_coroutine.h`
  - NOT USED (see below why)! 
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
    work
  - `callback`
    - captures a pointer
    - and a `void (*)(void* x) noexcept` function that takes that pointer
  - `make_member_callback`
    - helper function make a `callback` calling a member function for some
      class
  - `make_resume_coroutine_callback`
    - helper function to make a `callback` that resumes a coroutine handle
- `stop_util.h`
  - single threaded implementation of the standard variants
    - however compared with `std::stop_source` has better `noexcept`
      guarantees e.g. does not use a heap allocated state for `stop_source`
  - `stop_source` has a boolean that can be flipped to `true` using
    `request_stop()`
  - `stop_token` is really a pointer to `stop_source` to allow checking the
    state using `stop_requested()`
  - `stop_callback` calls a function object when the request to stop is called
    for the first time or when constructed. It uses a linked list provided
    by the `stop_source`
- `ready_queue.h`
  - `ready_queue` is an intrusive queue of ready work
  - `ready_node` the node in the queue contains:
    - `next`the pointer required for the queue
    - the work to be done as a function `fn`
      to be called with an `x` pointer as argument
- `timer_heap.h`
  - `timer_heap` is an intrusive heap of timers
  - `timer_node` the node in the heap contains:
    - pointers for `parent`, `left` and `right`
    - `deadline` for ordering by a monotonic `steady_clock` absolute time
    - the work to be done as a function `fn`
      to be called with an `x` pointer as argument
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
- `chain_context.h`
  - `chain_context` holds data for a cancellable chain:
    - the cancellation token
    - the continutation callback to run on completion of the chain
    - the cancellation callback to run when the chain was cancelled
    - a node that can be used to schedule callbacks
- `context.h`
  - `context`
    - holds references to:
      - an `event_loop_context`
      - a `chain_context`
    - except for the root context, the rest are created per chain
      by using the `event_loop_context` reference from the parent
      and a new `chain_context` (via a constructor)
- `event_loop.h`
  - `event_loop`
    - helper class holding a `ready_queue` and a `timer_heap`
    - `do_current_pending_work`
      - reads current pending tasks from both queue and heap and runs them
      - returns a duration to sleep if there is no more ready work, but
        there are pending timers
      - it's supposed to have some fairness e.g. we consume alternatively
        from both the `ready_queue` and the `timer_heap`
- // TODO `coro_type_traits.h`
- // TODO `run.h`
- `unique_coroutine_handle`
  - a RAII type owning a coroutine handle
- `promise_base`
  - base for coroutine promises to handle variations around the coroutine
    return value
  - template on a type `T`, specialized for `void`
  - handles `return_value`/`return_void` and `unhandled_exception`
  - provides `T get_result()` to either get the value or throw the exception
- // TODO `co.h`
- // TODO `yield.h`
- // TODO `sleep.h`
- // TODO `suspend_forever.h`
- // TODO `noop.h`
- // TODO `void_result.h`
- // TODO `wait_any_type_traits.h`
- // TODO `wait_any.h`
- // TODO `wait_all_type_traits.h`
- // TODO `wait_all.h`
- // TODO `wait_for.h`
- // TODO `task_list.h`