# What is `unique_handle`?

It's a RAII minimalist wrapper for C API handles (resource cleanup of resources coming from C
APIs). Think: like `std::unique_ptr`, but for C API handles.

`std::unique_handle` is really simple, minimalistic code; I recomment you have a look and read
it (ignore the concept definitions at the top on a first read, they are meant to help ensure
it's used correctly).

It's a template with a single parameter: a traits struct that configures it, instead of multiple
template parameters. You use a separate traits struct for each kind of resource to cleanup.
For explanation of why not `std::unique_ptr` and why the unusual traits approach see the design
decisions section below.

Given a C API like this:
```c
FILE* fopen(/*...*/); // 0 is the invalid value on failure
errno_t fopen_s(FILE**, /*...*/);
size_t fwrite(/*...*/, FILE*);
int fclose(FILE*);
```

Here is a simple/typical usage pattern for `unique_handle` and `handle_arg`
```cpp
struct file_handle_traits : cpp_util::unique_handle_out_ptr_access
{
  using handle_type = FILE*;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(std::fclose(h));
  }
};
using file_handle = cpp_util::unique_handle<file_handle_traits>;
using file_arg = cpp_util::handle_arg<file_handle_traits>;

// Construction API wrappers return a unique_handle to take care
// of the resource. Shown with fopen here, but often there are
// multiple such functions (e.g. wrapper for freopen)
file_handle open_file(const char* file_name, const char* mode)
{
  file_handle result{ std::fopen(file_name, mode) };
  if (!result.is_valid())
  {
    throw std::runtime_error("fopen failed");
  }
  return result;
}

// Usage functions. The `handle_arg` is used to avoid overloads that
// take the `unique_handle` or a raw `handle_type`. E.g. this allows usage
// with a non-owned raw `handle_type` such as `stdout` or `stderr`
void write_file(file_arg h, const char * buffer, size_t size)
{
  size_t write_count{ std::fwrite(buffer , 1, size, h) };
  if (write_count != size)
  {
    throw std::runtime_error("fwrite failed");
  }
}

// To handle errors in the closing function use the `close` idiom
// that throws exceptions outside destructors. See usage below.
void close_file(file_handle & x)
{
  int result = std::fclose(x.release());
  if (result != 0)
  {
    throw std::runtime_error("fclose failed");
  }
}

void usage()
{
  // Exception here does not return a RAII unique_handle
  auto file = open_file("foo", "wb");

  // Exception here will ensure fclose is called
  write_file(file, "bar", 3);
  write_file(file, "buzz", 4);

  // To handle flush failures,
  // take ownership of the handle and check result of fclose.
  // RAII unique_handle destructor will no longer call fclose
  close_file(file);
}
```

NOTE: throwing `std::runtime_error` is not always the best way to handle errors that might not
be exceptional for your application. Other options include using `std::error_code` or
`std::expected`. Even if exceptions are thrown, it's often better to use `std::system_error` that
includes the numerical error value. Using `std::runtime_error` in these examples for brevity, not
as a recommandation.

Many C APIs that initialize a handle do so by taking as an argument a pointer to the handle,
shown here reimplementing `open_file` to use `fopen_s`

```cpp
file_handle open_file(const char* file_name, const char* mode)
{
  file_handle return_value;
  auto result = std::fopen_s(return_value.out_ptr(), file_name, mode);
  if (0 != result)
  {
    throw std::runtime_error("fopen_s failed");
  }
  assert(return_value.is_valid());
  return return_value;
}
```

IMPORTANT: Note that the RAII object `return_value` is declared right before the C API.
This makes it easy to reason that the C API will get as input a pointer to an invalid handle
and that the RAII object will free the resource should `fopen_s` change the handle value.

The steps to use it for a C API that uses handles are:
- declare a struct for the traits
- inside the traits struct specify
  - `handle_type`: the type of the handle
  - `invalid_value`: the value that does not need to be closed
  - `close_handle`: how to close the handle/free resources
- inherit the traits struct from:
  - `unique_handle_out_ptr_access` if the C API that creates a handle takes a pointer
    to the handle as an out parameter (see `std::fopen_s` above)
  - `unique_handle_basic_access` if `.out_ptr()` is not needed (see `std::fopen` above)
  - `unique_handle_inout_and_out_ptr_access` to get `.out_ptr()`, `inout_ptr()` and
    `inout_ref()`
- alias a named type to `unique_handle` of the traits above
- optionally for custom checks for the valid value:
  - inside the traits declare `is_valid`
  - `static_assert` your traits against `unique_handle_custom_is_valid_traits` to ensure
    it's used (e.g. rather than mispelled)

# Usage samples

## POSIX close

- handle is not always a pointer
- invalid value is not always `0` or `nullptr`
- shows why choice of `close_handle` as a name: there are all sort of `close` functions already,
  less tricks required to ensure we call the right one
- inherits from `unique_handle_basic_access` as POSIX `open` returns the handle (rather than
  taking a pointer to the handle as a parameter)

```cpp
// e.g. in a posixlib namespace:
struct posix_file_handle_traits : cpp_util::unique_handle_basic_access
{
  using handle_type = int;
  static constexpr auto invalid_value() noexcept { return -1; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(close(h));
  }
};
using posix_file_handle = cpp_util::unique_handle<posix_file_handle_traits>;
```

## Windows typical handle

In Windows, `HANDLE` is opaque (that is it can be one of many things and the definition of what it is may
change over time), unlike the previous example where it's a `FILE *` (i.e. a pointer to a defined C `struct`).
A system API, as is the case of the Windows APIs, that want to protect itself against application misuse
taka a opaque handle that they first check against some hash map of opened handles before using, and it
might get translated into a pointer internally after this check, but the user of the API operates in terms
of this opaque handle which is not necessarily a pointer (though it might be a `void*`).

```cpp
// e.g. in a winlib namespace:
struct basic_handle_traits : cpp_util::unique_handle_out_ptr_access
{
  using handle_type = HANDLE;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(::CloseHandle(h));
  }
};
using basic_handle = cpp_util::unique_handle<basic_handle_traits>;
```


## Windows file handle uses INVALID_HANDLE_VALUE

- A file handle in Windows is also a `HANDLE`, but it's invalid value is `INVALID_HANDLE_VALUE`
- `INVALID_HANDLE_VALUE` uses `reinterpret_cast`, so `invalid_handle` is not `constexpr`.
- Often the underlying `handle_type` is reused by the underlying C API with different meanings.
  The traits approach creates different types to disambiguate betweeen those meanings when
  desired compared with configuring the RAII wrapper using multiple template parameters.

```cpp
// e.g. also in a winlib namespace:
struct file_handle_traits : cpp_util::unique_handle_out_ptr_access
{
  using handle_type = HANDLE;
  // notice lack of constexpr
  static auto invalid_value() noexcept { return INVALID_HANDLE_VALUE; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(::CloseHandle(h));
  }
};
using file_handle = cpp_util::unique_handle<file_handle_traits>;
```


## Windows any handle

- Declaring `is_valid` in the traits allows for specifying multiple "invalid"
  values
- Uses `static_assert` to make sure the traits meet the more speciffic concept
  (this is a compromise to avoid creating variations of `unique_handle` for when
  `is_valid` needs to be customized) and we did not mispelled it, or forgot
  the `noexcept` spec for the custom `is_valid`
- The approach of accepting multiple "invalid" values works only if we're convinced
  that truly there are multiple "invalid" values (e.g. that a C API that returns
  `NULL` will not return `INVALID_HANDLE_VALUE` as a valid handle and viceversa).
  E.g. useful to have a vector/array of `HANDLE`s that are passed to
  `::WaitForMultipleObjects`, agnostic of the precise invalid value.

```cpp
struct windows_any_handle_traits : cpp_util::unique_handle_out_ptr_access
{
  using handle_type = HANDLE;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  // notice lack of constexpr
  static auto is_valid(handle_type h) noexcept {
    return ( nullptr != h) && (INVALID_HANDLE_VALUE != h);
  }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(::CloseHandle(h));
  }
};
static_assert(cpp_util::unique_handle_custom_is_valid_traits<windows_any_handle_traits>);
using windows_any_handle = cpp_util::unique_handle<windows_any_handle_traits>;
```


## Windows device context associated to (GUI) window

- Custom `handle_type` that has muliple values required for `close_handle`.
  The handle is a `struct` holding those values. Note however that, although not unheard of,
  this is much rarer case in using C APIs, most functions only need a single value
  to free a resource.
- Shows the option to use `const handle_type & h` for `is_valid` and `close_handle` (this
  is to avoid passing two values instead of one which might be useful for larger structs,
  debateable if the struct has just two members as in this case)
- The constructor of `unique_handle` that accepts multiple arguments is
  useful in this scenario.

```cpp
struct window_dc_handle_traits : cpp_util::unique_handle_inout_and_out_ptr_access
{
  struct handle_type
  {
    HWND hwnd;
    HDC hdc;
  };
  static auto invalid_value() noexcept {
    return handle_type{ NULL, NULL };
  }
  static constexpr auto is_valid(const handle_type& h) noexcept {
    return NULL != h.hdc;
  }
  static void close_handle(const handle_type& h) noexcept {
    static_cast<void>(::ReleaseDC(h.hwnd, h.hdc));
  }
};
static_assert(cpp_util::unique_handle_custom_is_valid_traits<window_dc_handle_traits>);
using window_dc_handle = cpp_util::unique_handle<window_dc_handle_traits>;

window_dc_handle get_dc(HWND hwnd)
{
  window_dc_handle return_value{hwnd, ::GetDC(hwnd)};
  if (!return_value.is_valid())
  {
    throw std::runtime_error("GetDC failed");
  }
  return return_value;
}
```

Note that the there are more than one way of cleaning up a `HDC`, this one
does not need the `struct` for `handle_type`:
```cpp
struct dc_handle_traits : cpp_util::unique_handle_basic_access
{
  using handle_type = HDC hdc;

  static auto invalid_value() noexcept { return nullptr; }
  static void close_handle(const handle_type& h) noexcept {
    static_cast<void>(::DeleteDc(h));
  }
};
using dc_handle = cpp_util::unique_handle<dc_handle_traits>;

dc_handle create_compatible_dc(HDC dc)
{
  dc_handle return_value{ ::CreateCompatibleDC(dc)};
  if (!return_value.is_valid())
  {
    throw std::runtime_error("CreateCompatibleDC failed");
  }
  return return_value;
}
```

`.inout_ref()` might be useful to get a reference to these bigger `handle_type` structs,
instead of `.get()` which copies to struct in `window_dc_handle`,
which is why we derived `window_dc_handle_traits` from `unique_handle_inout_and_out_ptr_access`

```cpp
void foo()
{
  //...
  window_dc_handle wnd_dc = get_window_dc(::GetDesktopWindow());
  dc_handle memory_dc = create_compatible_dc(wnd_dc.inout_ref().hdc);
  //...
}
```


## Cleanup action

- sometimes the `unique_handle` just ensures some cleanup gets done
- shown here with a `bool` as the `handle_type`
- cleanup actions typically don't need pointer access to the `bool`, hence
  using `unique_handle_basic_access`

```cpp
struct revert_to_self_handle_traits : cpp_util::unique_handle_basic_access
{
  using handle_type = bool;
  static constexpr auto invalid_value() noexcept { return false; }
  static void close_handle(handle_type) noexcept {
    static_cast<void>(::RevertToSelf());
  }
};
using revert_to_self_handle = cpp_util::unique_handle<revert_to_self_handle_traits>;

revert_to_self_handle impersonate_anonymous()
{
  BOOL result = ::ImpersonateSelf(SecurityAnonymous);
  if (FALSE == result)
  {
    throw std::runtime_error("ImpersonateSelf failed");
  }
  return revert_to_self_handle(true);
}
```
IMPORTANT: creating these cleanup actions requires care e.g. to ensure above that
if `ImpersonateSelf` succeeds that there is no way to exit early without creating
a `revert_to_self_handle` (not that the exception is thrown for the failure case).

- therefore maybe use `BOOL` as the `handle_type`
```cpp
struct revert_to_self_handle_traits : cpp_util::unique_handle_basic_access
{
  using handle_type = BOOL;
  static constexpr auto invalid_value() noexcept { return FALSE; }
  static void close_handle(handle_type) noexcept {
    static_cast<void>(::RevertToSelf());
  }
};
using revert_to_self_handle = cpp_util::unique_handle<revert_to_self_handle_traits>;

revert_to_self_handle impersonate_anonymous()
{
  revert_to_self_handle result{ ::ImpersonateSelf(SecurityAnonymous) };
  if (!result.is_valid())
  {
    throw std::runtime_error("ImpersonateSelf failed");
  }
  return revert_to_self_handle(true);
}
```

- or use an enum as the `handle_type`
```cpp
enum class cleanup_action
{
  off,
  on
};
struct revert_to_self_handle_traits : cpp_util::unique_handle_basic_access
{
  using handle_type = cleanup_action;
  static constexpr auto invalid_value() noexcept { return cleanup_action::off; }
  static void close_handle(handle_type) noexcept {
    static_cast<void>(::RevertToSelf());
  }
};
using revert_to_self_handle = cpp_util::unique_handle<revert_to_self_handle_traits>;

revert_to_self_handle impersonate_anonymous()
{
  BOOL result = ::ImpersonateSelf(SecurityAnonymous);
  if (FALSE == result)
  {
    throw std::runtime_error("ImpersonateSelf failed");
  }
  return revert_to_self_handle(cleanup_action::on);
}
```

The cleanup action can be cancelled early using `reset()`.


## Cleanup action with data

- Sometimes cleanup also requires some data: demonstrating a combination of
  data (`ULONG flags`) and `bool`.
- The constructor of `unique_handle` that accepts multiple arguments is
  useful in this scenario

```cpp
struct http_terminate_handle_traits : cpp_util::unique_handle_basic_access
{
  struct handle_type
  {
    ULONG flags;
    bool enabled;
  };
  static constexpr auto invalid_value() noexcept { return handle_type{ 0, false }; }
  static constexpr bool is_valid(const handle_type& h) noexcept { return h.enabled; }
  static void close_handle(const handle_type& h) noexcept {
    static_cast<void>(::HttpTerminate(h.flags, nullptr));
  }
};
using http_terminate_handle = cpp_util::unique_handle<http_terminate_handle_traits>;

// this design assumes there are several functions that return a `http_terminate_handle`
// showing below one of them
http_terminate_handle initialize_v2_server()
{
  const ULONG flags = HTTP_INITIALIZE_SERVER;
  auto result = ::HttpInitialize(g_http_api_version_2, flags, nullptr);
  if (NO_ERROR != result)
  {
    throw std::runtime_error("HttpInitialize failed");
  }
  return http_terminate_handle(flags, true);
}
```


## Wrap `std::coroutine_handle`

- e.g. for use in a `task` that owns (and destroys) it
- we don't need a pointer access the the `coroutine_handle`, therefore
  using `unique_handle_basic_access`

```cpp
template<typename Promise>
struct unique_coroutine_handle_traits : cpp_util::unique_handle_basic_access
{
  using handle_type = std::coroutine_handle<Promise>;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static constexpr auto is_valid(handle_type h) noexcept { return h.operator bool(); }
  static void close_handle(handle_type h) noexcept { h.destroy(); }
};
template<typename Promise>
using unique_coroutine_handle = cpp_util::unique_handle<unique_coroutine_handle_traits<Promise>>;
```

usage:
```cpp
template<typename T>
class task
{
public:
  class promise_type
  {
    // ...
    task get_return_object() noexcept
    {
      return {std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    //...
  };
  private:
    unique_coroutine_handle<promise_type> unique_child_coro_;

    task(std::coroutine_handle<promise_type> child_coro) noexcept :
      unique_child_coro_{ child_coro }
    {
    }

    void resume()
    {
      unique_child_coro_.get().resume();
    }

    T await_resume()
    {
      return unique_child_coro_.get().promise().get_result();
    }
  // ...
};
```

## LocalFree wrapper templated example

- Shows usage where a templated version is used for `LocalFree`.
  Similar cases exist for `WTSFreeMemory`, `CoTaskMemFree` (also shown below) etc.
```cpp
template<typename T>
struct local_ptr_traits : cpp_util::unique_handle_out_ptr_access
{
  using handle_type = T;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept { static_cast<void>(::LocalFree(h)); }
};
template<typename T>
using local_ptr = cpp_util::unique_handle<local_ptr_traits<T>>;
```


## C-API uses struct with members that need cleaning

Shown below for `WINHTTP_INFO`, but similar for `STARTUPINFOW` and `PROCESS_INFORMATION`.
Also shows the usage of `.inout_ref()` to avoid copying the struct (this is a small struct, but
`STARTUPINFOW` and `PROCESS_INFORMATION` are larger, see futher example for that later).

e.g. given in `winhttp.h`, a Windows header that has this C struct
```cpp
typedef struct WINHTTP_INFO
{
  DWORD dwAccessType;
  LPWSTR lpszProxy;
  LPWSTR lpszProxyBypass;
};
```

Can be wrapped like this:
```cpp
struct http_proxy_info_handle_traits : cpp_util::unique_handle_out_ptr_access
{
  using handle_type = WINHTTP_PROXY_INFO;
  static auto invalid_value() noexcept {
    // this line zeroes the fields on the C struct
    return handle_type{};
  }
  static constexpr auto is_valid(const handle_type& h) noexcept {
    return (nullptr != h.lpszProxy) ||
      (nullptr != h.lpszProxyBypass);
  }
  static void close_handle(const handle_type& h) noexcept {
    free_string(h.lpszProxy);
    free_string(h.lpszProxyBypass);
  }
private:
  static void free_string(LPWSTR str) noexcept {
    if (nullptr != str) {
      ::GlobalFree(str);
    }
  }
};
static_assert(cpp_util::unique_handle_custom_is_valid_traits<http_proxy_info_handle_traits>);
using http_proxy_info_handle = cpp_util::unique_handle<http_proxy_info_handle_traits>;

void foo()
{
  // get system proxies
  http_proxy_info_handle system_proxies_info;
  if (!::WinHttpGetDefaultProxyConfiguration(system_proxies_info.out_ptr()))
  {
    // deal with, or ignore
  }
  WINHTTP_PROXY_INFO& raw = system_proxies_info.inout_ref();
  // use raw.lpszProxy and raw.lpszProxyBypass (if they are not nullptr)
}
```
One minor downside with this approach is that it raises the question if the compiler optimises
out the comparisons in `is_valid` check that preceeds `close_handle` because they are repeated in
 `free_string` called from `close_handle`.


## COM usage/support

We used this for some cases such as:

- COM appartment initialization scope
```cpp
struct co_uninitialize_handle_traits : cpp_util::unique_handle_basic_access
{
  using handle_type = bool;
  static constexpr auto invalid_value() noexcept { return false; }
  static void close_handle(handle_type) noexcept { ::CoUninitialize(); }
};
using co_uninitialize_handle = cpp_util::unique_handle<co_uninitialize_handle_traits>;

[[nodiscard]]
co_uninitialize_handle initialize_ex_multi_threaded()
{
  HRESULT hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (FAILED(hr))
  {
    throw std::runtime_error("CoInitializeEx(multithreaded) failed");
  }
  return co_uninitialize_handle(true);
}

[[nodiscard]]
co_uninitialize_handle initialize_ui_appartment()
{
  HRESULT hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  if (FAILED(hr))
  {
    throw std::runtime_error("CoInitializeEx(UI appartment) failed");
  }
  return co_uninitialize_handle(true);
}

void foo()
{
  auto com_init = initialize_ex_multi_threaded();
  // ... use COM here ...
}
```

- or a wrapper for `::CoTaskMemFree`
```cpp
template<typename T>
struct co_task_mem_free_handle_traits : cpp_util::unique_handle_out_ptr_access
{
  using handle_type = T;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept { ::CoTaskMemFree(h); }
};
template<typename T>
using co_task_mem_free_handle = cpp_util::unique_handle<co_task_mem_free_handle_traits<T>>;

std::wstring guid_to_wstring(const GUID& input)
{
  co_task_mem_free_handle<wchar_t*> str;
  HRESULT hr = ::StringFromCLSID(input, str.out_ptr());
  if (FAILED(hr))
  {
    throw std::runtime_error("StringFromCLSID failed");
  }
}
```

But we did not use it to wrap COM pointers, we use a different class for that because:
- We know that COM pointers are pointers
- If you do a lot of COM, you benefit from having a custom `operator->()` and helpers for
  creation and `QueryInterface` (e.g that can deal once with the required casts)
We also did not use it to wrap the COM BSTR, we use a different class for that because
a custom class:
- Can have additional constructos to create a BSTR wrapper from a `std::wstring` (via
  `SysAllocString`)
- Can have custom comparisons that do a deep compare of the strings pointed by BSTR


# Historical notes / why use `unique_handle` instead of `std::unique_ptr`

Here are some selective historical notes, that sadly does not explicitly acknowledge all the
contributors, it's not just me, I've learned from others.


## Pre C++23

Early 2000s I started switching from a C with classes coding style, with large and relatively monolithic
functions, to a more standard C++ one that embraced RAII that is correct in face of exceptions,
driven by the empyrical observation that using RAII to manage resources results in less code and code
with less bugs (in particular memory/resource leaks virtuall extinct). After disapointment with the,
now defunct, `std::auto_ptr` corner cases we ended up with a proliferation of non-copyable, non-moveable
classes, largely one for each resource.

When C++ 11 came along it introduced smart pointers like `std::unique_ptr` and `std::shared_ptr`.

For smart pointers there are all sort of options:
- is it reference counted?
- is the reference counting thread safe?
- is the deleter configurable at runtime?
- does it support (weak) reference counters that do not keep the data alive?

There are all sort of intermediate options (e.g. reference counted, but without weak counters and
maybe not thread safe and deleter configurable only at compile time), but `std::unique_ptr` is the
simplest option (answers "no" to the above options) while `std::shared_ptr` being the most
complex (it answers "yes" to the above options).

There are cognitive advantages from not providing too many options to a library user, so it's good
that the standard provided two choices from the opposite ends of the possible choices.

As a side thought, one can argue that even `std::shared_ptr` is a choice too many, allowing a naive
user to shoot themselves in the foot. How many times I've heard "I'm using `std::shared_ptr`
becaue I've heard it's good to use it" and I had to explain "Yes, compared with a naked pointer, but
it's better to use a `std::unique_ptr` instead if you can, and even better to not use a pointer
(smart or not) in the first place". How many times I've heard "I'm using `std::shared_ptr` because
it solves the lifetime issues", but it has viral effects on the codebase, only to then lead to
the need to mitigate against circular references. I used `std::shared_ptr` under duress in
conjunction with `boost::asio`. But I would argue that if you use `std::shared_ptr`, you probably
did not do your problem analysis right.

We thought that `std::unique_ptr` does not meet our needs for C API handles, in particular:
- not all handles were pointers
- the invalid value is not always `nullptr`
- did not have pointer access to the pointer
We could not figure out how to use `std::unique_ptr` to cover the first two issues, and the lack
of direct access to the pointer meant using temporary raw handle variables which felt like an
unnecessary risk.

For the last point instead of something like this (using a `file_handle` that gives pointer access
to the handle):
```cpp
file_handle open_file(const char* file_name, const char* mode)
{
  file_handle return_value;
  auto result = std::fopen_s(return_value.ptr(), file_name, mode);
  if (0 != result)
  {
    throw std::runtime_error("fopen_s failed");
  }
  assert(return_value.is_valid());
  return return_value;
}
```
If `file_handle` would have been `std::unique_ptr` based we would have to do something like
this is, which is more error prone.
```cpp
file_handle open_file(const char* file_name, const char* mode)
{
  FILE* raw_ptr = nullptr;
  auto result = std::fopen_s(&raw_ptr, file_name, mode);
  if (0 != result)
  {
    throw std::runtime_error("fopen_s failed");
  }
  file_handle return_value(raw_ptr);
  assert(return_value.is_valid());
  return return_value;
}
```

The other thing that happened is that C++11 introduced the convenient way to move types, and
we used that so that we can write smaller functions like the one above that calls the C API
and returns the smart class, decoupling the "normal C++" area from "calling a C API, be really
careful" areas, again with the empirical observation that this leads to more reusable code,
easier to reason for correctness.

And at this point we had several independent initiatives to consolidate writing such RAII
wrappers handles into reusable templates. Initially we configured them via template parameters,
but over time the single traits template parameter worked better.

But we still had multiple templates for what we thought are different cases:
- `raii_with_invalid_value` was the primary one, you specify in traits:
  - the `handle_type`: a type
  - the `invalid_value`: a value initially (later a function)
  - the `close_handle`: a function (`noexcept`)
- `raii_on_exit`
  - convered now by `unique_handle` with a `handle_type` `bool` or 2 valued `enum` (see example above)
- `raii_on_exit_with_data`
  - covered now by `unique_handle` with a `handle_type` a struct with a bool and extra fields
- `raii_with_data_and_invalid_value`
  - covered now by `unique_handle` e.g. the `HWND` + `HDC` struct (see example above)

It turns out that we got the most mileage out of `raii_with_invalid_value`, that I renamed to
`unique_handle`, and it could handle with some minimal friction the other cases as well.

**Reducing the cognitive overload** (the amount of things a developer needs to learn and remember) is
a important idea of library design. There are advantages from having a relatively simple recipe
that a developer can remember:

> If you use a C API that require some sort of freeing resource, strongly consider
> using `unique_handle`.


## C++23 `std::out_ptr` and `std::inout_ptr`

Then for C++23 we have `std::out_ptr` and `std::inout_ptr` that came in the standard from
[P1132R8](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1132r8.html).

By reading the paper I found out that other developers used `std::unique_ptr` to cover C API
handles.

For example the `FILE*` with a `nullptr` invalid value, this can be done relatively easily:
```cpp
struct file_ptr_deleter
{
  void operator()(FILE* handle) {
    static_cast<void>(std::fclose(handle));
  }
};
using file_ptr = std::unique_ptr<FILE*, file_ptr_deleter>;
```
If you fear: "what if `file_ptr` is created via `std::make_unique` and then freed using `delete`?",
fear not because`std::make_unique` cannot create a `std::unique_ptr` with a custom deleter,
it cannot create a `file_ptr` as above.

But if you want to have a handle `int` with a `-1` invalid value then
[P1132R8](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1132r8.html)
provides the following example:
```cpp
struct fd {
  int handle;

  fd() : fd(nullptr) {}
  fd(std::nullptr_t) : handle(static_cast<intptr_t>(-1)) {}
  fd(FILE* f)
#ifdef _WIN32
  : handle(f ? _fileno(f) : static_cast<intptr_t>(-1)){
#else
  : handle(f ? fileno(f) : static_cast<intptr_t>(-1)) {
#endif // Windows
  }

  explicit operator bool() const;

  bool operator==(std::nullptr_t) const;
  bool operator!=(std::nullptr_t) const;
  bool operator==(const fd& fd) const;
  bool operator!=(const fd& fd) const;
};

struct fd_deleter {
  using pointer = fd;
  void operator()(fd des) const;
};

std::unique_ptr<int, fd_deleter> my_unique_fd;
auto err = fopen_s( std::out_ptr<FILE*>(my_unique_fd), "prod.csv", "rb" );;
```
On one side this shows that people tried and can use `std::unique_ptr` for an `int` handle with
a `-1` invalid value, but I would call it an unpleasant experience compared with `unique_handle`,
at least a very differet experiece in writing a wrapper.

The key is that if the deleter for a `unique_ptr` has a `pointer` type, that really overrides
what `unique_ptr` stores, in particular the `int` in `std::unique_ptr<int, ...` is meaningless
as far as I can see. This is burried in the spec for `unique_ptr`:
> If the qualified-id `remove_reference_t<D>::pointer` is valid and denotes a type (13.10.3),
> then `unique_ptr<T, D>::pointer` shall be a synonym for `remove_reference_t<D>::pointer`.
> Otherwise `unique_ptr<T, D>::pointer` shall be a synonym for `element_type*`

Also this example is incorrect: why `static_cast<intptr_t>(-1)` rather than just `-1` to assign
to an `int` (`intptr_t` is an integral type that is as large as a pointer, not necessarily `int`)?.

But it shows the issue of cognitive overload: writing a `unique_ptr` for a pointer handle is simple,
but for a non-pointer handle it's significantly more complex, the example is not even complete, many
functions are declared, but not implemented in the example.

What we would have done using `unique_handle` is have two handle classes:
```cpp
struct file_handle_traits : cpp_util::unique_handle_out_ptr_access
{
  using handle_type = FILE*;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(std::fclose(h));
  }
};
using file_handle = cpp_util::unique_handle<file_handle_traits>;

struct file_descriptor_traits : cpp_util::unique_handle_basic_access
{
  using handle_type = int;
  static constexpr auto invalid_value() noexcept { return -1; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(close(h));
  }
};
using file_descriptor = cpp_util::unique_handle<file_descriptor_traits>;
```

Notice how similar is to write these wrappers, and then we would write
a function to convert from one to the other which takes care of the case
where `fileno` fails:
```cpp
file_descriptor to_file_descriptor(file_handle& f)
{
  file_descriptor return_value{
#ifdef _WIN32
  _fileno(f.get())
#else
  fileno(f.get())
#endif // Windows
  };
  if (!return_value.is_valid())
  {
    // maybe even std::terminate, if we believe this case
    // is a coding mistake
    throw std::runtime_error("fileno failed");
  }
  else
  {
    static_cast<void>(f.release());
  }
  return return_value;
}
```

Coming back to `out_ptr` and `inout_ptr`, they are for using C APIs that take a pointer
to the handle. The difference between `out_ptr` and `inout_ptr` is that:
- `out_ptr` is for C APIs like `fopen_s`, taking a pointer to the handle, not
  caring about the input value (e.g. it could be invalid, and often it is),
  setting it on successful return to a created resource.
- `inout_ptr` is for C APIs that also take a pointer to a handle as a parameter,
  but the handle value is checked, they might free the resource inside the C API
  and then create return a new handle via the same parameter.

The `out_ptr` style C APIs are way more common than the `inout_ptr`. `out_ptr` is really
way more more common:
- While I don't deny that `inout_ptr` style C APIs dealing with a handle exist,
  I can't remember one `inout_ptr`, my co-workers can't remember one, the best we could come
  up with is "There could have been some in Windows COM".
- If you check the documentation for
  [SAL annotations](https://learn.microsoft.com/en-us/cpp/code-quality/understanding-sal?view=msvc-170)
  (SAL is Microsoft's source code annotation language): it contains `_Outptr_`, but it
  does not contain `_InOutPtr_` or similar

Implementation-wise `out_ptr` is suposed to `.reset()` the smart pointer before the
C API is called. The differentiation is supposed to ensure the C APIs are called correctly.

If using one when the other should be used:
- If `out_ptr` is used when calling a `inout_ptr` style C API, the C API will get an
  invalid/closed value and the outcome will be different from the intended one (e.g.
  the C API might return an error).
- If `inout_ptr` is used when calling a `out_ptr` style C API, the resource will be
  leaked, **if the smart pointer holds a valid value**, which is the very thing these
  libraries try to avoid.

But in practice, developers get away with accidental usage of `inout_ptr` when calling a `out_ptr`
style C API is mittigated, if they just declare the smart pointer, default constructed, just
before calling the `out_ptr` C API, i.e. if they don't reuse an existing smart pointer, but
create one each time it's needed.

In fact for years out `raii_with_invalid_value` only had a `.ptr()` member used as shown above:
```cpp
  file_handle return_value;
  // reset would be redundat there
  auto result = std::fopen_s(return_value.ptr(), file_name, mode);
```

To the designers of [P1132R8](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1132r8.html)
it seems it was important to cover `std::shared_ptr` in addition to `std::unique_ptr`.
Unfortunately this has some unpallatable consequences. I want to make clear that
if I use hard words, they addressed to the technical outcome, not to people (I am
actually thankful to the authors of P1132R8 providing insights into their design
thoughts).

`std::out_ptr` and `std::inout_ptr` return proxy types that store a reference to the
smart pointer and a temporary that is manipulated at construction and destruction of
the proxy type, both calling `.reset()` on the smart pointer in their destructor.

The issue with the `std::shared_ptr` is that its `.reset()` can throw, because it might
fail to allocate the state the stores the counters and the deleter that `.reset()` could
provide. Faced with a throwing destructor the designers of P1132R8 could have gone back
to the design of the solution: if you need to create/allocate resources in the destructor
it's a smell. Luckily at least someone stopped them from allowing exceptions from the
destructor, though the design means that using `std::out_ptr` or `std::inout_ptr` could
result in a program termination in face of a `std::bad_alloc` exception. For me this is
not a pallatable library contract.

For a simpler, non-reference counted types like `std::unique_ptr` and `unique_handle` the
proxy type is not required:
- `inout_ptr()` can just return a pointer to the pointer/handle
- `out_ptr()` can just call `.reset()` before returning a pointer to the pointer/handle

The authors P1132R8 considered that providing such metods directly would break encapsulation:
> The problem is this breaks encapsulation over its knee and destroys and integrity the
> pointer value has from unique_ptr's invariant.

I disagree. The above sounds like OOP arguments for getting setters and getters for
`std::vector` elements. That does not really hide access, it just raises performance
questions, by not providing an afficient basis of operations for the type:

> A basis is efficient if and only if any procedure implemented using it is
> as efficient as an equivalent procedure written in terms of an alternative basis.
> (from "Elements of Programming" chapter 1.4)

For a `std::unique_ptr` one can manipulate the pointer (indirectly):
```cpp
auto proxy_object = std::inout_ptr(smart_pointer);
auto ptr = static_cast<Type**>(proxy_object);
*ptr = /*...*/
```
P1132R8 has tries to addres questions about performance via a whole section of the
paper, which would either not be a problem or one easier to aswer if a proxy type is
not used.

The proxy type also creates novel ways to shoot yourself in the foot compared
with direct access:
```cpp
std::unique_ptr<foo_handle, foo_deleter> my_unique(nullptr);

if (get_some_pointer(std::out_ptr(my_unique)) && my_unique)) {
// lifetime of proxy means that my_unique.reset(raw) is not called
// by the time my_unique is tested
// though P1132R8 uses the below `if` as a problem example, but
// that's fine, because the temporary is destructed at the ; preceeding
// the test of my_unique:
//if (get_some_pointer(std::out_ptr(my_unique)); my_unique)) {
  std::cout << "yay" << std::endl;
}
else {
  std::cout << "oh no" << std::endl;
}
```

Also `out_ptr` and `inout_ptr` have some pointer conversions that are fine if they
work, but also introduce novel ways to get it wrong.

The last issue to answer: how would one then return a `std::shared_ptr` to a
`unique_handle` and avoid termination? Again, with the caveats that you've probably
not done the analysis right, asuming `file_handle` is based on `unique_handle`,
then something like this:
```cpp
std::shared_ptr<file_handle> open_file_as_shared(/*...*/)
{
  file_handle h = open_file(/*...*/);
  return std::make_shared<file_handle>(std::move(h));
}
```

Having said all that, I'm likely to use `unique_handle` to deal with C API handles
for the forseeable future instead of using `std::unique_ptr`/`std::out_ptr`/`std::inout_ptr`.


# Design decisions

## Why not use `std::unique_ptr` for handles?

As a summary of the historical notes: because handles stretch `std::unique_ptr`
outside it's primary usage scenario, which is pointers with `nullptr` as the invalid
value.


## Why traits instead of taking template parameters

1. allows the case where the invalid value is a `reinterpret_cast`

2. allows options for disambiguation, see choices for `SC_HANDLE` below

- `SC_HANDLE` is one of the many examples of a ambiguous usage in a C API: it could be a service, or it
  could be a service manager (required to open a service)

- one can design an API wrapper that accepts this ambiguity:
```cpp
struct service_handle_traits : cpp_util::unique_handle_basic_access
{
  using handle_type = SC_HANDLE;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(::CloseServiceHandle(h));
  }
};
using service_handle = cpp_util::unique_handle<service_handle_traits>;
using service_arg = cpp_util::handle_arg<service_handle_traits>;

service_handle open_service_manager(DWORD dwDesiredAccess);

service_handle open_service(
  service_arg smgr, const std::wstring & service_name, DWORD desired_access);
```

- or one can design an API wrapper that disambigutes between the usages.
  NOTE: the traits approach creates different types for `service_manager_handle`
  and `service_handle` even if the traits body is the same.
```cpp
struct service_manager_handle_traits : cpp_util::unique_handle_basic_access
{
  using handle_type = SC_HANDLE;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(::CloseServiceHandle(h));
  }
};
using service_manager_handle = cpp_util::unique_handle<service_manager_handle_traits>;
using service_manager_arg = cpp_util::handle_arg<service_manager_handle_traits>;

service_manager_handle open_service_manager(DWORD dwDesiredAccess);

struct service_handle_traits : cpp_util::unique_handle_basic_access
{
  using handle_type = SC_HANDLE;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(::CloseServiceHandle(h));
  }
};
using service_handle = cpp_util::unique_handle<service_handle_raii_traits>;

service_handle open_service(
  service_manager_arg smgr, const std::wstring & service_name, DWORD desired_access);
```

3. Makes this disambiguation controlled by the C++ code as shown for `SC_HANDLE` when compared with an implementation where the three elements (handle type, invalid value and function to close handle) are taken as template arguments.

Q: are type1 and type2 different types?
```cpp
using type1 = some_other_raii_handle<void*, nullptr, LocalMemFree>;
using type2 = some_other_raii_handle<PSECURITY_DESCRIPTOR, NULL, LocalMemFree>;
```

A: Don't know, it depends on what `PSECURITY_DESCRIPTOR` and `NULL` are.

C handles are opaque. `PSECURITY_DESCRIPTOR` could be:
- a pointer to a `struct` called `SECURITY_DESCRIPTOR`

- a `void*`
- an integer
- a pointer sized integral type
- a struct containing a pointer sized member
- something else, none of the above


## Traits still look unusual

Well, maybe as a name, but if you compare with the of `std::unique_ptr` there
are similarities:

<table border="1">
  <thead>
    <tr>
      <td>unique_ptr deleter</td>
      <td>unique_handle traits</td>
      <td>Notes:</td>
   </tr>
  </thead>
  <tbody>
    <tr>
      <td>pointer</td>
      <td>handle_type</td>
      <td>Configure the type of the pointer/handle</td>
    </tr>
    <tr>
      <td>pointer type constructor from `nullptr`</td>
      <td>invalid_value()</td>
      <td>Configure the invalid value</td>
    </tr>
    <tr>
      <td>pointer type operator `bool`</td>
      <td>is_valid()</td>
      <td>Test for the invalid value</td>
    </tr>
    <tr>
      <td>operator()</td>
      <td>close_handle()</td>
      <td>Configure how to free the resource</td>
    </tr>
  </tbody>
</table>


## `.ref()`, `.ptr()`, `.out_ptr()` etc. 

The predecesor of `unique_handle`, `raii_with_invalid_value` provided
just `.ptr()`, without the `out` and `inout` variations.

This was not much of a problem as most of the C APIs are `out_ptr` type
ones, the usage was to declare an instance just ahead of calling the
C API.
```cpp
file_handle open_file(const char* file_name, const char* mode)
{
  file_handle return_value;
  auto result = std::fopen_s(return_value.ptr(), file_name, mode);
  //...
  return return_value;
}
```

I provided `.ref()` because I thought this was part of an "expressive basis"
(see EOP 1.4), but it turns out that there are not so many usages for that
e.g. maybe for a large struct like `STARTUPINFOW` (see below).

On the other side the experience shows that developers are more likely
to misuse it. Most of the usages in the field of `.ref()` would immediately
just take the address like `&smart_ptr.ref()`: it would have been easier to
just use `.ptr()` like `smart_ptr.ptr()`.

Also some developers expect a distinct `out_ptr()` access
```cpp
file_handle open_file(const char* file_name, const char* mode)
{
  file_handle return_value;
  auto result = std::fopen_s(return_value.out_ptr(), file_name, mode);
  //...
  return return_value;
}
```
and are concerned of a direct access via a `.ptr()`. Not sure if the concern
is justified, but I see that it's real, people are concerned.

So I've added a mechanism to use the traits to selecting available pointer
access in `unique_handle`.

C APIs creating handles often just return a handle e.g.:
```cpp
file_handle result{ std::fopen(file_name, mode) };
```
in which case no direct pointer is required in `file_handle`.

For this the traits class can be derived from `cpp_util::unique_handle_basic_access`:
```cpp
struct file_handle_traits : cpp_util::unique_handle_basic_access
```

The basic access in `unique_handle` is similar to `std::unique_ptr`
and consists of:
- handle can be provided in the constructor
- `.get()` obtains a copy of the handle
- handle can be changed via `.reset()` or move from another
- ownershit is relinquished via `.release()`

The basic access is the recommended option for cleanup actions that store a
`bool` (or similar) as a handle.

More modern C APIs creating handles return an error code, the handle to be
created is taken by pointer and `out_ptr()` access provides the interface
to do this:
```cpp
file_handle return_value;
auto result = std::fopen_s(return_value.out_ptr(), file_name, mode);
```
For this the traits class should be derived from `cpp_util::unique_handle_out_ptr_access`:
```cpp
struct file_handle_traits : cpp_util::unique_handle_out_ptr_access
```
And this should be the default option to use unless yoy have a better reason.

If the traits class is derived from `cpp_util::unique_handle_inout_and_out_ptr_access`
then `.inout_ptr()` and `.inout_ref()` are added on top of the basic access
and `.out_ptr()`:
- `.inout_ptr()` for those elusive `inout_ptr` style C APIs
  - that's really a renamed `.ptr()`
- `.inout_ref()` for effective access avoiding the copy via `.get()`
  - that's really a renamed `.ref()`


## Why no `.out_ref()`?

The asymmetry that there is a `.inout_ref()`, but no `.out_ref()` is because while there are
very few cases where you might want a reference (e.g. when the "handle" is a struct, to
avoid copying the struct), I've not encountered a case where you want to `.reset()` before
access, usually the `.reset()` did happen when the handle was initialized via a `out_ptr`
C API.


## Performance of `.out_ptr()` vs direct pointer access

How does using `.out_ptr()` like this:
```cpp
file_handle open_file(const char* file_name, const char* mode)
{
  file_handle return_value;
  auto result = std::fopen_s(return_value.out_ptr(), file_name, mode);
  if (0 != result)
  {
    throw std::runtime_error("fopen_s failed");
  }
  return return_value;
}
```
compares with just using the pointer:
```cpp
file_handle open_file(const char* file_name, const char* mode)
{
  file_handle return_value;
  auto result = std::fopen_s(return_value.ptr(), file_name, mode);
  if (0 != result)
  {
    throw std::runtime_error("fopen_s failed");
  }
  return return_value;
}
```
The difference is that `.out_ptr()` unnecessaryly re-assigns the
`invalid_value()` to the handle inside the `unique_handle`, to
one that the default constructor already assigned the `invalid_value()`.

As far as I could see, using the compiler explorer at https://godbolt.org/,
this re-assignment is optimised out by the compiler in `/O2`/release
builds.

But if this is your concern, you can use `.inout_ptr()`, which is just
`.ptr()` really. We have `.out_ptr()` to address the concern of those
worried that the handle might not be set to `invalid_value()` before
calling the C API.

## Performance of `unique_handle` vs C

How does it compare with the C alternative:
```c
bool foo(const char* file_name, const char* mode)
{
  FILE* file;
  errno_t result;
  result = fopen_s(&file, file_name, mode);
  if (0 != result)
  {
    return false;
  }

  fclose(file);
  return true;
}
```
The particular C version above avoids testing `file` before closing, it only tested
the `result` and assumes that the `file` is not `nullptr`. It might gain something
out of that, though it's playing with fire. NOTE: this is a trick similar to the
one used in linear find value with sentinel equal to the value where the loop only
has to test for the sentinel, not for the end, compared with the `std::find` that
needs to test for end and value in the loop.

Performance-wise the `unique_ptr` C++ version tests the `FILE*` in addition to the
`result` and requires the compiler to optmise out the move operations.

The advantages of the `unique_ptr` C++ version are that it:
- allows reuse: you only need to wrap `fopen_s` once in your application
- prevents coding errors where the file is not closed on some return branch


## Why use handle_arg, why not use raw handles?

In the example above we had
```cpp
service_handle open_service(
  service_manager_arg smgr, const std::wstring & service_name, DWORD desired_access);
```

It creates a barrier when we want to distinguish between the `SC_HANDLE`
returned from `OpenSCManagerW` and the one returned by `OpenServiceW`.
The barrier is permeable as the `service_manager_arg` can also be constructed
with a raw `SC_HANDLE` that we can obtain by calling `get()` on a `service_handle`,
but it is still a barrier.


## Comparison (equality in particular) and std::hash

`std::unique_ptr` implements equality, but it does not make sense. The
only instances that are equal have a `nullptr` value. The values that are
not `nullptr` better be unique. Another smell is that a equality is
linked to copy (a copy should be equal to the original), but `std::unique_ptr`
is not copyable.

`unique_handle` is in a similar situation. I did not implement equality,
comparison or `std::hash`.

There is potentially the case where one would want to implement a C API
in C++ and provide handles via that C API, and needs order or hash to
efficiently search for opened handles in some data structure. Well,
`unique_handle` was not designed for that, it was designed for using
C APIs that deal with handles rather than implementing them.


## swap
Did not yet have to implement, could be, if needed.


## Design of constraints in unique_handle_traits

```cpp
  template<unique_handle_traits Traits>
  class [[nodiscard]] unique_handle
```

`unique_handle_traits` is a concept that tries to enforce requirements on
`unique_handle` trait types e.g. to ensure that `close_handle` is a `noexcept`
function (because it's called from the destructor of `unique_handle`) or
that the `handle_type` is constructible without exceptions (this is
what `get` does)

There is an additional `unique_handle_custom_is_valid_traits` that can be
used via a `static_assert` to check that `is_valid` is provided correctly
by the traits class: this was a compromise to avoid duplicating `unique_handle`
for that scenario.


## Micro design decissions

### Why traits inheritance for access configuration

E.g.
```cpp
struct file_handle_traits : cpp_util::unique_handle_out_ptr_access
{
  //...
}
```
I know it's unusual, but it's brief.

We would have used a type definition:
```cpp
struct file_handle_traits
{
  // ...
  using access = cpp_util::unique_handle_out_ptr_access;
}
```

We would have used a compile time constant (a `enum class` value):
```cpp
struct file_handle_traits
{
  // ...
  static constexpr auto access = cpp_util::unique_handle_access::out_ptr;
}
```

Both are more verbose.


### Move constructor
Move constructor is implented as:
```cpp
unique_handle(unique_handle&& other) noexcept :
  h_{ std::move(other.h_) }
{
  other.h_ = Traits::invalid_value();
}
```
It could be re-written as
```cpp
unique_handle(unique_handle&& other) noexcept :
  h_{ std::exchange(other.h_, Traits::invalid_value()) }
{
}
```
But in the first case it's clear that exactly two assignments take place, whereas
in the `exchange` case we rely on the compiler to optimise out, and did not want to
risk it especially for the case where the `handle_type` is a large C struct.

### Move assignment
Similarly
```cpp
unique_handle& operator=(unique_handle&& other) noexcept
{
  handle_type tmp = std::move(other.h_);
  other.h_ = Traits::invalid_value();
  close_if_valid();
  h_ = std::move(tmp);
  return *this;
}
```
vs. the more compact
```cpp
unique_handle& operator=(unique_handle&& other) noexcept
{
  reset(other.h_.release());
  return *this;
}
```
It's harder to count the assignments in the `reset`+`release` version.

Also note that we avoided a version that checks `if (&other == this)` out of
concern of additional branching, the code as it is handles the assignment to self
because if `other` is this very object, then the value is saved to `tmp` and
assigning the `invalid_value` to `other.h_` will make this very handle invalid,
with nothing to close before restoring `h_` from `tmp`.


# Even more usage scenarios

## WTSFreeMemory wrapper templated example
- Shows usage where a templated version is used for `::WTSFreeMemory`.
- It also shows a case where the `out_ptr()` method is used
  for an API where a `reinterpret_cast` is needed.

```cpp
template<typename T>
struct wts_free_memory_raii_traits : cpp_util::unique_handle_out_ptr_access
{
  using handle_type = T;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept { ::WTSFreeMemory(h); }
};
template<typename T>
using wts_free_memory_raii = cpp_util::unique_handle<wts_free_memory_raii_traits<T>>;

template<typename T>
wts_free_memory_raii<T> query_session_information(DWORD sessionId, WTS_INFO_CLASS wtsInfoClass)
{
  wts_free_memory_raii<T> return_value;

  DWORD bytesReturned = 0;
  BOOL result = ::WTSQuerySessionInformationW(
    WTS_CURRENT_SERVER_HANDLE, sessionId, wtsInfoClass,
    reinterpret_cast<LPWSTR>(return_value.out_ptr()),
    &bytesReturned);
  if (FALSE == result)
  {
    throw std::runtime_error("WTSQuerySessionInformation failed");
  }

  return return_value;
}

void usage()
{
  // ...
  std::wstring wts_user_name = query_session_information<WCHAR*>(session_id, WTSUserName).get();

  connect_state state = to_connect_state(
    *query_session_information<WTS_CONNECTSTATE_CLASS*>(session_id, WTSConnectState).get());
  // ...
}
```


## Dealing with CreateProcessW structures

This shows:
- How to wrap a C API struct that has multiple resources inside
- How to compose such unique handles:
  - Have a unique handle for each C API struct
  - Then have a struct that has those unique handle as members
- How to use `.inout_ref()` to avoid further copies compared with `.get()`
  - Though the compiler should probably optise those copies out

```cpp
struct startup_info_handle_traits : cpp_util::unique_handle_out_ptr_access
{
  using handle_type = STARTUPINFOW;

  static constexpr auto invalid_value() noexcept {
    return handle_type{ .cb = sizeof(STARTUPINFOW) };
  }
  static constexpr bool is_valid(const handle_type& h) noexcept {
    return (nullptr != h.hStdInput) ||
      (nullptr != h.hStdOutput) ||
      (nullptr != h.hStdError);
  }
  static void close_handle(const handle_type& h) noexcept {
    close_handle_helper(h.hStdInput);
    close_handle_helper(h.hStdOutput);
    close_handle_helper(h.hStdError);
  }
private:
  static void close_handle_helper(HANDLE h) noexcept {
    if (nullptr != h) {
      static_cast<void>(::CloseHandle(h));
    }
  }
};
static_assert(cpp_util::unique_handle_custom_is_valid_traits<startup_info_handle_traits>);
using startup_info_handle = cpp_util::unique_handle<startup_info_handle_traits>;

struct process_information_handle_traits : cpp_util::unique_handle_out_ptr_access
{
  using handle_type = PROCESS_INFORMATION;

  static constexpr auto invalid_value() noexcept {
    return handle_type{};
  }
  static constexpr bool is_valid(const handle_type& h) noexcept {
    return (nullptr != h.hProcess) ||
      (nullptr != h.hThread);
  }
  static void close_handle(const handle_type& h) noexcept {
    close_handle_helper(h.hProcess);
    close_handle_helper(h.hThread);
  }
private:
  static void close_handle_helper(HANDLE h) noexcept {
    if (nullptr != h) {
      static_cast<void>(::CloseHandle(h));
    }
  }
};
static_assert(cpp_util::unique_handle_custom_is_valid_traits<process_information_handle_traits>);
using process_information_handle = cpp_util::unique_handle<process_information_handle_traits>;

struct created_process
{
  startup_info_handle startup_info;
  process_information_handle process_information;
};

created_process create_process(/*...*/)
{
  created_process return_value;

  BOOL result = ::CreateProcessW(
    //...
    return_value.startup_info.out_ptr(),
    return_value.process_information.out_ptr()
  );
  if (FALSE == result)
  {
    throw std::runtime_error("CreateProcessW failed");
  }

  return return_value;
}

int run_and_wait(/*...*/)
{
  created_process proc = create_process(/*...*/);
  return wait_for_process_exit(proc.process_information.inout_ref().hProcess);
}
```

## Compounding cleanup for the same handle

- This is a rarer case where multiple cleanups are required for the same handle, one can store two instances of the `unique_handle`, or can compond both into one that can be passed arround withouth having to store the handle twice.

```cpp
struct win_http_handle_traits : cpp_util::unique_handle_basic_access
{
  using handle_type = HINTERNET;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(::WinHttpCloseHandle(h));
  }
};
using win_http_handle = cpp_util::unique_handle<win_http_handle_traits>;

struct unregister_callback_handle_traits : cpp_util::unique_handle_basic_access
{
  using handle_type = HINTERNET;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept {
    WINHTTP_STATUS_CALLBACK callback_result{ ::WinHttpSetStatusCallback(h, NULL, /*...*/) };
    if (WINHTTP_INVALID_STATUS_CALLBACK == callback_result)
    {
      // this is a fatal error (there is bug somewhere)
      // throw will terminate, because we're inside noexcept function
      throw std::runtime_error("WinHttpSetStatusCallback fatal error");
    }
  }
};
using unregister_callback_handle = cpp_util::unique_handle<unregister_callback_handle_traits>;

// compond handle cleans up both of the above
struct win_http_session_handle_traits : cpp_util::unique_handle_basic_access
{
  using handle_type = HINTERNET;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept {
    // the destructors (in reverse order) will do the cleanup
    win_http_handle a(h);
    unregister_callback_handle b(h);
  }
};
using win_http_session_handle = cpp_util::unique_handle<win_http_session_handle_traits>;

win_http_session_handle transfer_to_win_http_session_handle(
  win_http_handle && http_handle,
  unregister_callback_handle && unregister_callback) noexcept
{
  // throw will terminate (there is bug somewhere)
  if((unregister_callback.get() != http_handle.get()) || !(http_handle.is_valid()))
  {
    throw std::runtime_error("transfer_to_win_http_session_handle fatal error");
  }
  static_cast<void>(unregister_callback.release());
  return win_http_session_handle(http_handle.release());
}

// Users only care about win_http_session which stores a single HINTERNET
win_http_session_handle open_session()
{
  win_http_handle handle{ ::WinHttpOpen(/*...*/) }
  if (!handle.is_valid())
  {
    throw std::runtime_error("WinHttpOpen failed");
  }

  // ...

  WINHTTP_STATUS_CALLBACK callback_result{
    ::WinHttpSetStatusCallback(handle.get(), winhttp_status_callback, /*...*/) };
  if (WINHTTP_INVALID_STATUS_CALLBACK == callback_result)
  {
    throw std::runtime_error("WinHttpSetStatusCallback failed");
  }
  unregister_callback_handle unregister_callback(handle.get());

  // ...

  return transfer_to_win_http_session_handle(
    std::move(handle), std::move(unregister_callback));
}
```

## The case of multiple cleanup functions

This is a very rare case e.g. `PSID` in Windows:

When obtained from `AllocateAndInitializeSid` must be freed by using `FreeSid`.
```cpp
BOOL AllocateAndInitializeSid(
  // ...
  [out] PSID *pSid
);
```

When obtained from `ConvertStringSidToSidW` must be freed by using `LocalFree`.
```cpp
BOOL ConvertStringSidToSidW(
  // ...
  [out] PSID *Sid
);
```

When obtained from `CreateWellKnownSid` can use our own method of allocating and
freeing (including space on the stack). E.g. can call once with a `NULL`, then use
`cbSid` as the number of bytes that need to be allocated.
```cpp
BOOL CreateWellKnownSid(
  // ...
  [out, optional] PSID pSid,
  [in, out] DWORD *cbSid
);
```

In other cases the `PSID` is allocated as part of a larger structure.
E.g. for `GetTokenInformation` the user has to allocate a buffer. When
called with `TokenUser`, the content of the buffer is nominally a struct
```cpp
struct TOKEN_USER
{
  SID_AND_ATTRIBUTES User;
};
```
where
```cpp
struct SID_AND_ATTRIBUTES
{
  PSID  Sid;
  DWORD Attributes;
};
```
But the buffer needs to be larger to accomodate the `SID` pointed by `Sid`,
following the `TOKEN_USER`.

I speculate that the reason for the `PSID` multiple cleanup functions is that
it is not a true resource (e.g. like a file), but instead it is a pointer to
[a C data structure](https://learn.microsoft.com/en-us/windows-server/identity/ad-ds/manage/understand-security-identifiers)
(despite the `PSID` being declared as a `void*`).

The options are to either:
- use a single RAII class based on `local_ptr<PSID>`, never use `AllocateAndInitializeSid`
- use multiple RAII classes
  - e.g. one based on `local_ptr<PSID>`, another one that calls `FreeSid` and maybe yet
    another one storing a `unique_ptr<unsigned char[]>`
- use a special RAII that captures the cleanup/deleter as a pointer

The last two options require a custom `sid_arg` type rather than one based on `handle_arg`
which was designed to work in conjunction with a single `unique_handle`
