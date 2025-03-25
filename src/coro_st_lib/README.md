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
- `stop_util.h`
  - single threaded implementation of the standard variants
  - `stop_source` has a boolean that can be flipped to `true` using
    `request_stop()`
  - `stop_token` is really a pointer to `stop_source` to allow checking the
    state using `stop_requested()`
  - `stop_callback` calls a function object when the request to stop is called
    for the first time or when constructed. It uses a linked list provided
    by the `stop_source`
- `synthetic_coroutine.h`
  - allows building synthetic coroutine handles i.e. assuming the ABI for a coroutine
    frame starts with the resume and destroy functions
  - `synthetic_resumable_coroutine_frame` calls a callback (function and pointer) when
    `resume()` is called on the handle obtained with `get_coroutine_handle()`