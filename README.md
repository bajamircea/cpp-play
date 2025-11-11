
https://lewissbaker.github.io/2017/11/17/understanding-operator-co-await#synchronisation-free-async-code

Raymond Chen's "The Old New Thing" blog: "C++ coroutines: The problem of the DispatcherQueue task that runs too soon, part 4"
https://devblogs.microsoft.com/oldnewthing/20191226-00/?p=103268

ASAN output from EXAMPLE 1
```
=================================================================
==10116==ERROR: AddressSanitizer: heap-use-after-free on address 0x113ec83a0088 at pc 0x7ff71739232b bp 0x000f59f3f490 sp 0x000f59f3f498
WRITE of size 8 at 0x113ec83a0088 thread T0
==10116==WARNING: Failed to use and restart external symbolizer!
    #0 0x7ff71739232a in std::coroutine_handle<void>::coroutine_handle<void> C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\coroutine:92
    #1 0x7ff717392eaf in std::coroutine_handle<void>::from_address C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\coroutine:66
    #2 0x7ff7173926d2 in std::coroutine_handle<std::noop_coroutine_promise>::operator std::coroutine_handle<void> C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\coroutine:202
    #3 0x7ff717392bf4 in awaiter_destroy_in_await_suspend::await_suspend C:\Users\builder\Desktop\repro\main.cpp:95
    #4 0x7ff7173917b5 in async_coro$_ResumeCoro$1 C:\Users\builder\Desktop\repro\main.cpp:105
    #5 0x7ff717393439 in std::coroutine_handle<task::promise_type>::resume C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\coroutine:140
    #6 0x7ff717393481 in task::start C:\Users\builder\Desktop\repro\main.cpp:76
    #7 0x7ff717391466 in main C:\Users\builder\Desktop\repro\main.cpp:113
    #8 0x7ff717394e58 in invoke_main D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:78
    #9 0x7ff717394da1 in __scrt_common_main_seh D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:288
    #10 0x7ff717394c5d in __scrt_common_main D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:330
    #11 0x7ff717394ecd in mainCRTStartup D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_main.cpp:16
    #12 0x7ff92d2be8d6 in BaseThreadInitThunk+0x16 (C:\WINDOWS\System32\KERNEL32.DLL+0x18002e8d6)
    #13 0x7ff92deac53b in RtlUserThreadStart+0x2b (C:\WINDOWS\SYSTEM32\ntdll.dll+0x18008c53b)

0x113ec83a0088 is located 72 bytes inside of 176-byte region [0x113ec83a0040,0x113ec83a00f0)
freed by thread T0 here:
    #0 0x7ff717394063 in operator delete D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_win_delete_scalar_size_thunk.cpp:41
    #1 0x7ff717391bed in async_coro$_ResumeCoro$1 C:\Users\builder\Desktop\repro\main.cpp:107
    #2 0x7ff717391524 in async_coro$_DestroyCoro$2 C:\Users\builder\Desktop\repro\main.cpp:15732480
    #3 0x7ff717392d97 in std::coroutine_handle<task::promise_type>::destroy C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\coroutine:144
    #4 0x7ff7173925b6 in task::~task C:\Users\builder\Desktop\repro\main.cpp:62
    #5 0x7ff717392906 in task::`scalar deleting destructor'+0x16 (C:\Users\builder\Desktop\repro\x64\Debug\repro.exe+0x140002906)
    #6 0x7ff7173928cb in std::default_delete<task>::operator() C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\memory:3309
    #7 0x7ff717393329 in std::unique_ptr<task,std::default_delete<task> >::reset C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\memory:3471
    #8 0x7ff717392bcd in awaiter_destroy_in_await_suspend::await_suspend C:\Users\builder\Desktop\repro\main.cpp:91
    #9 0x7ff7173917b5 in async_coro$_ResumeCoro$1 C:\Users\builder\Desktop\repro\main.cpp:105
    #10 0x7ff717393439 in std::coroutine_handle<task::promise_type>::resume C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\coroutine:140
    #11 0x7ff717393481 in task::start C:\Users\builder\Desktop\repro\main.cpp:76
    #12 0x7ff717391466 in main C:\Users\builder\Desktop\repro\main.cpp:113
    #13 0x7ff717394e58 in invoke_main D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:78
    #14 0x7ff717394da1 in __scrt_common_main_seh D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:288
    #15 0x7ff717394c5d in __scrt_common_main D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:330
    #16 0x7ff717394ecd in mainCRTStartup D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_main.cpp:16
    #17 0x7ff92d2be8d6 in BaseThreadInitThunk+0x16 (C:\WINDOWS\System32\KERNEL32.DLL+0x18002e8d6)
    #18 0x7ff92deac53b in RtlUserThreadStart+0x2b (C:\WINDOWS\SYSTEM32\ntdll.dll+0x18008c53b)

previously allocated by thread T0 here:
    #0 0x7ff717393fd5 in operator new D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_win_new_scalar_thunk.cpp:40
    #1 0x7ff717391074 in async_coro C:\Users\builder\Desktop\repro\main.cpp:15732480
    #2 0x7ff7173913d0 in main C:\Users\builder\Desktop\repro\main.cpp:112
    #3 0x7ff717394e58 in invoke_main D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:78
    #4 0x7ff717394da1 in __scrt_common_main_seh D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:288
    #5 0x7ff717394c5d in __scrt_common_main D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:330
    #6 0x7ff717394ecd in mainCRTStartup D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_main.cpp:16
    #7 0x7ff92d2be8d6 in BaseThreadInitThunk+0x16 (C:\WINDOWS\System32\KERNEL32.DLL+0x18002e8d6)
    #8 0x7ff92deac53b in RtlUserThreadStart+0x2b (C:\WINDOWS\SYSTEM32\ntdll.dll+0x18008c53b)

SUMMARY: AddressSanitizer: heap-use-after-free C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\coroutine:92 in std::coroutine_handle<void>::coroutine_handle<void>
Shadow bytes around the buggy address:
  0x113ec839fe00: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x113ec839fe80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x113ec839ff00: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x113ec839ff80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x113ec83a0000: fa fa fa fa fa fa fa fa fd fd fd fd fd fd fd fd
=>0x113ec83a0080: fd[fd]fd fd fd fd fd fd fd fd fd fd fd fd fa fa
  0x113ec83a0100: fa fa fa fa fa fa fd fd fd fd fd fd fd fd fd fd
  0x113ec83a0180: fd fd fd fd fd fd fd fd fd fd fd fa fa fa fa fa
  0x113ec83a0200: fa fa fa fa fd fd fd fd fd fd fd fd fd fd fd fd
  0x113ec83a0280: fd fd fd fd fd fd fd fd fd fa fa fa fa fa fa fa
  0x113ec83a0300: fa fa fd fd fd fd fd fd fd fd fd fd fd fd fd fd
Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Partially addressable: 01 02 03 04 05 06 07
  Heap left redzone:       fa
  Freed heap region:       fd
  Stack left redzone:      f1
  Stack mid redzone:       f2
  Stack right redzone:     f3
  Stack after return:      f5
  Stack use after scope:   f8
  Global redzone:          f9
  Global init order:       f6
  Poisoned by user:        f7
  Container overflow:      fc
  Array cookie:            ac
  Intra object redzone:    bb
  ASan internal:           fe
  Left alloca redzone:     ca
  Right alloca redzone:    cb
==10116==ABORTING

C:\Users\builder\Desktop\repro\x64\Debug\repro.exe (process 10116) exited with code 1 (0x1).
Press any key to close this window . . .
```

ASAN output from EXAMPLE 2
```
=================================================================
==15328==ERROR: AddressSanitizer: heap-use-after-free on address 0x120b021a01f8 at pc 0x7ff75f1e203b bp 0x007d1a17f7c0 sp 0x007d1a17f7c8
WRITE of size 8 at 0x120b021a01f8 thread T0
==15328==WARNING: Failed to use and restart external symbolizer!
    #0 0x7ff75f1e203a in std::coroutine_handle<void>::coroutine_handle<void> C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\coroutine:92
    #1 0x7ff75f1e2dff in std::coroutine_handle<void>::from_address C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\coroutine:66
    #2 0x7ff75f1e2472 in std::coroutine_handle<std::noop_coroutine_promise>::operator std::coroutine_handle<void> C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\coroutine:202
    #3 0x7ff75f1e2a30 in task::promise_type::final_awaiter::await_suspend C:\Users\builder\Desktop\repro\main.cpp:163
    #4 0x7ff75f1e1739 in async_do_nothing$_ResumeCoro$1 C:\Users\builder\Desktop\repro\main.cpp:211
    #5 0x7ff75f1e3409 in std::coroutine_handle<task::promise_type>::resume C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\coroutine:140
    #6 0x7ff75f1e34c2 in task::start C:\Users\builder\Desktop\repro\main.cpp:204
    #7 0x7ff75f1e13e3 in main C:\Users\builder\Desktop\repro\main.cpp:217
    #8 0x7ff75f1e4e98 in invoke_main D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:78
    #9 0x7ff75f1e4de1 in __scrt_common_main_seh D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:288
    #10 0x7ff75f1e4c9d in __scrt_common_main D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:330
    #11 0x7ff75f1e4f0d in mainCRTStartup D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_main.cpp:16
    #12 0x7ff92d2be8d6 in BaseThreadInitThunk+0x16 (C:\WINDOWS\System32\KERNEL32.DLL+0x18002e8d6)
    #13 0x7ff92deac53b in RtlUserThreadStart+0x2b (C:\WINDOWS\SYSTEM32\ntdll.dll+0x18008c53b)

0x120b021a01f8 is located 56 bytes inside of 128-byte region [0x120b021a01c0,0x120b021a0240)
freed by thread T0 here:
    #0 0x7ff75f1e40a3 in operator delete D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_win_delete_scalar_size_thunk.cpp:41
    #1 0x7ff75f1e190b in async_do_nothing$_ResumeCoro$1 C:\Users\builder\Desktop\repro\main.cpp:211
    #2 0x7ff75f1e14a4 in async_do_nothing$_DestroyCoro$2 C:\Users\builder\Desktop\repro\main.cpp:15732480
    #3 0x7ff75f1e2bd7 in std::coroutine_handle<task::promise_type>::destroy C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\coroutine:144
    #4 0x7ff75f1e2356 in task::~task C:\Users\builder\Desktop\repro\main.cpp:189
    #5 0x7ff75f1e26a6 in task::`scalar deleting destructor'+0x16 (C:\Users\builder\Desktop\repro\x64\Debug\repro.exe+0x1400026a6)
    #6 0x7ff75f1e266b in std::default_delete<task>::operator() C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\memory:3309
    #7 0x7ff75f1e32f9 in std::unique_ptr<task,std::default_delete<task> >::reset C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\memory:3471
    #8 0x7ff75f1e2a09 in task::promise_type::final_awaiter::await_suspend C:\Users\builder\Desktop\repro\main.cpp:161
    #9 0x7ff75f1e1739 in async_do_nothing$_ResumeCoro$1 C:\Users\builder\Desktop\repro\main.cpp:211
    #10 0x7ff75f1e3409 in std::coroutine_handle<task::promise_type>::resume C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\coroutine:140
    #11 0x7ff75f1e34c2 in task::start C:\Users\builder\Desktop\repro\main.cpp:204
    #12 0x7ff75f1e13e3 in main C:\Users\builder\Desktop\repro\main.cpp:217
    #13 0x7ff75f1e4e98 in invoke_main D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:78
    #14 0x7ff75f1e4de1 in __scrt_common_main_seh D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:288
    #15 0x7ff75f1e4c9d in __scrt_common_main D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:330
    #16 0x7ff75f1e4f0d in mainCRTStartup D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_main.cpp:16
    #17 0x7ff92d2be8d6 in BaseThreadInitThunk+0x16 (C:\WINDOWS\System32\KERNEL32.DLL+0x18002e8d6)
    #18 0x7ff92deac53b in RtlUserThreadStart+0x2b (C:\WINDOWS\SYSTEM32\ntdll.dll+0x18008c53b)

previously allocated by thread T0 here:
    #0 0x7ff75f1e4015 in operator new D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_win_new_scalar_thunk.cpp:40
    #1 0x7ff75f1e1070 in async_do_nothing C:\Users\builder\Desktop\repro\main.cpp:15732480
    #2 0x7ff75f1e133b in main C:\Users\builder\Desktop\repro\main.cpp:216
    #3 0x7ff75f1e4e98 in invoke_main D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:78
    #4 0x7ff75f1e4de1 in __scrt_common_main_seh D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:288
    #5 0x7ff75f1e4c9d in __scrt_common_main D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:330
    #6 0x7ff75f1e4f0d in mainCRTStartup D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_main.cpp:16
    #7 0x7ff92d2be8d6 in BaseThreadInitThunk+0x16 (C:\WINDOWS\System32\KERNEL32.DLL+0x18002e8d6)
    #8 0x7ff92deac53b in RtlUserThreadStart+0x2b (C:\WINDOWS\SYSTEM32\ntdll.dll+0x18008c53b)

SUMMARY: AddressSanitizer: heap-use-after-free C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\include\coroutine:92 in std::coroutine_handle<void>::coroutine_handle<void>
Shadow bytes around the buggy address:
  0x120b0219ff00: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x120b0219ff80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x120b021a0000: fa fa fa fa fa fa fa fa 00 00 00 00 00 00 00 00
  0x120b021a0080: 00 00 00 00 00 00 00 02 fa fa fa fa fa fa fa fa
  0x120b021a0100: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 02 fa
=>0x120b021a0180: fa fa fa fa fa fa fa fa fd fd fd fd fd fd fd[fd]
  0x120b021a0200: fd fd fd fd fd fd fd fd fa fa fa fa fa fa fa fa
  0x120b021a0280: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x120b021a0300: fa fa fa fa fa fa fa fa 00 00 00 00 00 00 00 00
  0x120b021a0380: 00 00 00 00 00 00 00 fa fa fa fa fa fa fa fa fa
  0x120b021a0400: fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd fa
Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Partially addressable: 01 02 03 04 05 06 07
  Heap left redzone:       fa
  Freed heap region:       fd
  Stack left redzone:      f1
  Stack mid redzone:       f2
  Stack right redzone:     f3
  Stack after return:      f5
  Stack use after scope:   f8
  Global redzone:          f9
  Global init order:       f6
  Poisoned by user:        f7
  Container overflow:      fc
  Array cookie:            ac
  Intra object redzone:    bb
  ASan internal:           fe
  Left alloca redzone:     ca
  Right alloca redzone:    cb
==15328==ABORTING

C:\Users\builder\Desktop\repro\x64\Debug\repro.exe (process 15328) exited with code 1 (0x1).
Press any key to close this window . . .
```