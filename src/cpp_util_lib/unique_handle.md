# Usage samples for `unique_handle`

## cstdio file handle

Shows simple/typical usage pattern for `unique_handle` and `handle_arg`

```cpp
// e.g. in a cstdio namespace:
struct file_handle_traits {
  using handle_type = FILE*;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(std::fclose(h));
  }
};
using file_handle = cpp_util::unique_handle<file_handle_traits>;
using file_arg = cpp_util::handle_arg<file_handle>;

// Construction API wrappers return a unique_handle to take care
// of the resource. Shown with fopen here, but often there are
// multiple such functions (e.g. wrapper for freopen)
file_handle open_file(const char* file_name, const char* mode)
{
  file_handle result{ std::fopen(file_name, mode) };
  if (!result.is_valid())
  {
    error::throw_errno("fopen");
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
    error::throw_errno("fwrite");
  }
}

// To handle errors in the closing function use the `close` idiom
// that throws exceptions outside destructors. See usage below.
void close_file(file_handle & x)
{
  int result = std::fclose(x.release());
  if (result != 0)
  {
    error::throw_failed("fclose");
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

## Direct access case

Some APIs want direct access to the handle, shown here reimplementing to use `fopen_s`.
For this declare a `unique_handle` and use `handle_pointer()`.

```cpp
file_handle open_file(const char* file_name, const char* mode)
{
  file_handle return_value;
  auto result = std::fopen_s(return_value.handle_pointer(), file_name, mode);
  if (0 != result)
  {
    error::throw_errno(result, "fopen_s");
  }
  assert(return_value.is_valid());
  return return_value;
}
```

## POSIX close

- handle is not always a pointer
- invalid value is not always `0` or `nullptr`
- shows why choice of `close_handle`: there are all sort of `close` functions already,
  less tricks required to ensure we call the right one

```cpp
// e.g. in a posixlib namespace:
struct posix_file_handle_traits {
  using handle_type = int;
  static constexpr auto invalid_value() noexcept { return -1; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(close(h));
  }
};
using posix_file_handle = cpp_util::unique_handle<posix_file_handle_traits>;
```

## Windows typical handle

- in Windows `HANDLE` is opaque (that is it can be one of many things and the definition of what it is may change over time), unlike the previous example where it's a `FILE *` (i.e. a pointer to a defined C `struct`).

```cpp
// e.g. in a winlib namespace:
struct basic_handle_traits {
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
  desired.

```cpp
// e.g. also in a winlib namespace:
struct file_handle_traits {
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

- `is_valid` allows multiple "invalid" values (or, see further exaples,
  customization on what it means to be valid)
- uses `static_assert` to make sure the traits meet the more speciffic concept
- This approach works if we're convinced that there are truly
  multiple "invalid" values (e.g. that a C API that returns `NULL` will not return
  `INVALID_HANDLE_VALUE` as a valid handle). E.g. useful to have a vector/array
  of `HANDLE`s that are passed to `::WaitForMultipleObjects`, agnostic of the precise
  invalid value.

```cpp
// e.g. also in a winlib namespace:
struct windows_any_handle_traits {
  using handle_type = HANDLE;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  // notice lack of constexpr
  static auto is_valid(handle_type h) noexcept {
    return (h != nullptr) || (h != INVALID_HANDLE_VALUE);
  }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(::CloseHandle(h));
  }
};
static_assert(cpp_util::unique_handle_custom_is_valid_traits<windows_any_handle_traits>);

using windows_any_handle = cpp_util::unique_handle<windows_any_handle_traits>;
```

## Windows device context associated to (GUI) window

- Custom `handle_type` that has muliple values required for `close_handle`. Note however that, although not unheard of, this is much rarer case in using C APIs, most functions only need a single value.
- Shows the option to use `const handle_type & h` for `is_valid` and `close_handle`
- The constructor of `unique_handle` that accepts multiple arguments is
  useful in this scenario.
- `.ref()` might be useful to get a reference to these bigger `handle_type` structs.

```cpp
struct window_dc_handle_traits {
  struct handle_type {
    HWND hwnd;
    HDC hdc;
  };
  static auto invalid_value() noexcept {
    return handle_type{ NULL, NULL };
  }
  static constexpr auto is_valid(const handle_type& h) noexcept {
    return h.hdc != NULL;
  }
  static void close_handle(const handle_type& h) noexcept {
    static_cast<void>(::ReleaseDC(h.hwnd, h.hdc));
  }
};
using window_dc_handle = cpp_util::unique_handle<window_dc_handle_traits>;

window_dc_handle get_dc(HWND hwnd) {
  window_dc_handle return_value{hwnd, ::GetDC(hwnd)};
  if (!return_value.is_valid()) {
    throw std::runtime_error("GetDC failed");
  }
  return return_value;
}
```

## Cleanup action

- sometimes the `unique_handle` just ensures some cleanup gets done
- shown here with a `bool` as the `handle_type`

```cpp
struct revert_to_self_handle_traits {
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
  error::throw_gle_if_zero(result, "ImpersonateSelf");
  return revert_to_self_handle(true);
}
```

- or use `BOOL` as the `handle_type`
```cpp
struct revert_to_self_handle_traits {
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
    error::throw_gle_error("ImpersonateSelf");
  }
  return revert_to_self_handle(true);
}
```

- or use an enum as the `handle_type`
```cpp
enum class cleanup_action {
  off,
  on
};
struct revert_to_self_handle_traits {
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
  error::throw_gle_if_zero(result, "ImpersonateSelf");
  return revert_to_self_handle(cleanup_action::on);
}
```

The cleanup action can be cancelled early using `reset()`.


# Cleanup action with data

- Sometimes cleanup also requires some data: demonstrating a combination of
  data (`ULONG flags`) and `bool`.
- The constructor of `unique_handle` that accepts multiple arguments is
  useful in this scenario

```cpp
struct http_terminate_handle_traits {
  struct handle_type {
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
http_terminate_handle initialize_v2_server() {
  const ULONG flags = HTTP_INITIALIZE_SERVER;
  auto result = ::HttpInitialize(g_http_api_version_2, flags, nullptr);
  throw_http_on_error(result, "HttpInitialize");
  return http_terminate_handle(flags, true);
}
```

# Wrap `std::coroutine_handle`

- e.g. for use in a `task` that owns (and destroys) it

```cpp
template<typename Promise>
struct unique_coroutine_handle_traits
{
  using handle_type = std::coroutine_handle<Promise>;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static constexpr auto is_valid(handle_type h) noexcept { return h.operator bool(); }
  static void close_handle(handle_type h) noexcept { h.destroy(); }
};
template<typename Promise>
using unique_coroutine_handle = cpp_util::unique_handle<unique_coroutine_handle_traits<Promise>>;
```

# Templated example

- Shows usage where a templated version is used for `::WTSFreeMemory`.
  Similar cases exist for `LocalFree` (also shown below), `CoTaskMemFree` etc.
- It also shows a case where the `handle_pointer()` method is used
  for API that takes a pointer to the handle, albeit a complex one
  where a `reinterpret_cast` is also used. For a simpler case see
  `RegOpenKeyExW`.

```cpp
template<typename T>
struct wts_free_memory_raii_traits {
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
    reinterpret_cast<LPWSTR>(return_value.handle_pointer()),
    &bytesReturned);
  error::throw_if_false_getlasterror(result, "WTSQuerySessionInformation");

  return return_value;
}

void usage()
{
  // ...
  std::wstring wts_user_name = query_session_information<WCHAR*>(session_id, WTSUserName).get();

  connect_state state = to_connect_state(*query_session_information<WTS_CONNECTSTATE_CLASS*>(session_id, WTSConnectState).get());
  // ...
}
```

Sample of `LocalFree` wrapper
```cpp
template<typename T>
struct local_ptr_traits {
  using handle_type = T;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept { static_cast<void>(::LocalFree(h)); }
};
template<typename T>
using local_ptr = cpp_util::unique_handle<local_ptr_traits<T>>;
```

# Why traits instead of taking template parameters

1. allows the case where the invalid value is a `reinterpret_cast`

2. allows options for disambiguation, see choices for `SC_HANDLE` below

- `SC_HANDLE` is one of the many examples of a ambiguous usage in a C API: it could be a service, or it
  could be a service manager (required to open a service)

- one can design an API wrapper that accepts this ambiguity:
```cpp
struct service_handle_traits {
  using handle_type = SC_HANDLE;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(::CloseServiceHandle(h));
  }
};
using service_handle = cpp_util::unique_handle<service_handle_traits>;

service_handle open_service_manager(DWORD dwDesiredAccess);

service_handle open_service(
  const service_handle& smgr, const std::wstring & service_name, DWORD desired_access);
```

- or one can design an API wrapper that disambigutes between the usages.
  NOTE: the traits approach creates different types for `service_manager_handle`
  and `service_handle` even if the traits body is the same.
```cpp
struct service_manager_handle_traits {
  using handle_type = SC_HANDLE;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(::CloseServiceHandle(h));
  }
};
using service_manager_handle = cpp_util::unique_handle<service_manager_handle_traits>;

service_manager_handle open_service_manager(DWORD dwDesiredAccess);

struct service_handle_traits {
  using handle_type = SC_HANDLE;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(::CloseServiceHandle(h));
  }
};
using service_handle = cpp_util::unique_handle<service_handle_raii_traits>;

service_handle open_service(
  const service_manager_handle& smgr, const std::wstring & service_name, DWORD desired_access);
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


# Compounding cleanup for the same handle

- This is a rarer case where multiple cleanups are required for the same handle, one can store two instances of the `unique_handle`, or can compond both into one that can be passed arround withouth having to store the handle twice.

```cpp
struct win_http_handle_traits {
  using handle_type = HINTERNET;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(::WinHttpCloseHandle(h));
  }
};
using win_http_handle = cpp_util::unique_handle<win_http_handle_traits>;

struct unregister_callback_handle_traits {
  using handle_type = HINTERNET;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static void close_handle(handle_type h) noexcept {
    WINHTTP_STATUS_CALLBACK callback_result{ ::WinHttpSetStatusCallback(h, NULL, /*...*/) };
    // throw will terminate, this is a fatal error
    throw_if_getlasterror(WINHTTP_INVALID_STATUS_CALLBACK == callback_result, "WinHttpSetStatusCallback");
  }
};
using unregister_callback_handle = cpp_util::unique_handle<unregister_callback_handle_traits>;

// compond handle cleans up both of the above
struct win_http_session_handle_traits {
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
  // throw will terminate
  throw_if((unregister_callback.get() != http_handle.get()) || !(http_handle.is_valid()),
    "transfer_to_win_http_session_handle");
  static_cast<void>(unregister_callback.release());
  return win_http_session_handle(http_handle.release());
}

// Users only care about win_http_session which stores a single HINTERNET
win_http_session_handle open_session() {
  win_http_handle handle{ ::WinHttpOpen(/*...*/) }
  throw_if_getlasterror(!handle.is_valid(), "WinHttpOpen");

  // ...

  WINHTTP_STATUS_CALLBACK callback_result{
    ::WinHttpSetStatusCallback(handle.get(), winhttp_status_callback, /*...*/) };
  throw_if_getlasterror(WINHTTP_INVALID_STATUS_CALLBACK == callback_result, "WinHttpSetStatusCallback");
  unregister_callback_handle unregister_callback(handle.get());

  // ...

  return transfer_to_win_http_session_handle(
    std::move(handle), std::move(unregister_callback));
}
```

# C-API case not handled by `unique_handle`

Custom RAII classes are still required when dealing with C-APIs.
Here is an example that needs to clean up to pointers.

```cpp
struct http_proxy_info {
  WINHTTP_PROXY_INFO data{
    .dwAccessType=0,
    .lpszProxy=0,
    .lpszProxyBypass=0
  };

  http_proxy_info() noexcept = default;

  http_proxy_info(const http_proxy_info&) = delete;
  http_proxy_info& operator=(const http_proxy_info&) = delete;

  ~http_proxy_info() {
    free_string(data.lpszProxy);
    free_string(data.lpszProxyBypass);
  }

private:
  void free_string(LPWSTR str) noexcept {
    if (str != nullptr) {
      ::GlobalFree(str);
    }
  }
};
```

# The case of multiple cleanup functions

This is a very rare case e.g. `PSID` in Windows:
```cpp
BOOL AllocateAndInitializeSid(
  [in]  PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
  //...
  [in]  DWORD                     nSubAuthority7,
  [out] PSID                      *pSid
);
// A SID allocated with the AllocateAndInitializeSid function
// must be freed by using the FreeSid function.

BOOL ConvertStringSidToSidW(
  [in]  LPCWSTR StringSid,
  [out] PSID    *Sid
);
// [out] Sid: A pointer to a variable that receives a pointer to the
// converted SID. To free the returned buffer, call the LocalFree function.
```

The options are to either
- use two RAII classes (e.g. one based on `local_ptr<PSID>` and
another one that calls `FreeSid`)
- or use a special RAII that captures the cleanup/deleter as a pointer
