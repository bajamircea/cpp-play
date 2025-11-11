# GCC dangling pointer warning

This branch contains info on reproducing (what I believe to be) a false positive from GCC.

## Summary

Using coroutines to implement cancellable tasks has the risk of coding mistakes where pointers are genuinely
dangling. The reproduction code in `src/repro_test/main.cpp` when compiled with `-Wdangling-pointer -O3` (in addition to `-std=c++23`):
```
src/repro_test/main.cpp:518:48: error: storing the address of local variable ‘co_awaiter’ in ‘*(co::promise_type*)((char*)SR + 16).co::promise_type::pctx_’ [-Werror=dangling-pointer=]
  518 |       unique_child_coro_.get().promise().pctx_ = &ctx;
      |       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~~
```
The code is fairly complicated. It is a snippet from a real project that runs a test that involves a coroutine
used to implement a lazy task (`co`).

I believe the issue is a false positive because:
- Albeit reasoning requires knowledge of what the library tries to achieve, one can reason
  that "The pointer comes from a reference `context& ctx` that outlives the `awaiter`,
  which owns (and outlives) the coroutine frame where the promise lives
  (where the pointer is stored)"
- The warning is only on `-O3`, does not happen on non-optimized code
- The warning goes away if the code is simplified further in ways unrelated to the reported problem.
  E.g. just run the test function instead of looping over the one test as the test framework does:
  then the warning is not shown.
- The warning message is incorrect: it mentions storing the address of `co_awaiter` when we
  actually store a `ctx`. You would expect it would complain of `ctx` or `main_stop_source` not
  of `co_awaiter`


## Compiler output:
```
g++ --std=c++23 -fno-rtti -MT int/release/repro_test/main.o -MMD -MP -MF int/release/repro_test/main.d -Wall -Wpedantic -Wextra -Werror -O3 -march=native -g -c -o int/release/repro_test/main.o src/repro_test/main.cpp
In constructor ‘co::awaiter::awaiter(context&, unique_coroutine_handle<co::promise_type>&&)’,
    inlined from ‘co::awaiter co::work::get_awaiter(context&)’ at src/repro_test/main.cpp:565:49,
    inlined from ‘stop_when_task<CoTask1, CoTask2>::awaiter::awaiter(context&, stop_when_task<CoTask1, CoTask2>::CoWork1&, stop_when_task<CoTask1, CoTask2>::CoWork2&) [with CoTask1 = suspend_forever_task; CoTask2 = co]’ at src/repro_test/main.cpp:709:7,
    inlined from ‘stop_when_task<CoTask1, CoTask2>::awaiter stop_when_task<CoTask1, CoTask2>::work::get_awaiter(context&) [with CoTask1 = suspend_forever_task; CoTask2 = co]’ at src/repro_test/main.cpp:802:40,
    inlined from ‘void run(CoTask) [with CoTask = stop_when_task<suspend_forever_task, co>]’ at src/repro_test/main.cpp:926:8:
src/repro_test/main.cpp:518:48: error: storing the address of local variable ‘co_awaiter’ in ‘*(co::promise_type*)((char*)SR + 16).co::promise_type::pctx_’ [-Werror=dangling-pointer=]
  518 |       unique_child_coro_.get().promise().pctx_ = &ctx;
      |       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~~
src/repro_test/main.cpp: In function ‘void run(CoTask) [with CoTask = stop_when_task<suspend_forever_task, co>]’:
src/repro_test/main.cpp:926:8: note: ‘co_awaiter’ declared here
  926 |   auto co_awaiter = co_task.get_work().get_awaiter(ctx);
      |        ^~~~~~~~~~
src/repro_test/main.cpp:920:17: note: ‘co_task’ declared here
  920 | void run(CoTask co_task)
      |          ~~~~~~~^~~~~~~
cc1plus: all warnings being treated as errors
make: *** [makefile:73: int/release/repro_test/main.o] Error 1
```
