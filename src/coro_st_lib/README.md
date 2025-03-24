Single threaded coroutine framework. It's useful as a learning tool
and as a benchmark against which threading support can be measured.

All the code is in the `coro_st` namespace.

- `promise_base.h`
  - base for coroutine promises to handle variations around the coroutine
    return value
  - template on a type `T`, specialized for `void`
  - handles `return_value`/`return_void` and `unhandled_exception`
  - provides `T get_result()` to either get the value or throw the exception
