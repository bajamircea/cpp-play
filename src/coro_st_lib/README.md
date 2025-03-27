Single threaded coroutine framework. It's useful as a learning tool
and as a benchmark against which threading support can be measured,
but the single threadness is quite a limitation to actual real usage.

All the code is in the `coro_st` namespace.

- `unique_coroutine_handle`
  - a RAII type owning a coroutine handle
- `promise_base`
  - base for coroutine promises to handle variations around the coroutine
    return value
  - template on a type `T`, specialized for `void`
  - handles `return_value`/`return_void` and `unhandled_exception`
  - provides `T get_result()` to either get the value or throw the exception
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
- `stop_util.h`
  - single threaded implementation of the standard variants
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
      advantage that nodes can be removed in `O(1)` (e.g. when a timer
      in cancelled) and that insertion in particular can be made `noexcept`
      which is useful to be able to use a timer in error recovery
      scenarios: e.g. on exception sleep for a while and then try again,
      that would be problematic if sleeping could throw
- `event_loop_context.h`
  - holds references to the ready queue and heap and allows
    - adding node to ready queue
    - adding node to the timer heap
    - removing node from timer heap (e.g. when timer cancelled)
