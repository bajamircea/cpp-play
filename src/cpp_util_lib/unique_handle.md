# Usage samples for `unique_handle`

## cstdio file handle

Shows simple/typical usage pattern for `unique_handle` and `handle_arg`

```cpp
// e.g. in a cstdio namespace:
struct file_handle_traits {
  using handle_type = FILE *;
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
file_handle open(const char * file_name, const char * mode)
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
void write(file_arg h, const char * buffer, size_t size)
{
  size_t write_count{ std::fwrite(buffer , 1, size, h) };
  if (write_count != size)
  {
    error::throw_errno("fwrite");
  }
}

// To handle errors in the closing function use the `close` idiom
// that throws exceptions outside destructors. See usage below.
void close(file_handle & x)
{
  int result = std::fclose(x.release());
  if (result != 0)
  {
    error::throw_failed("fclose");
  }
}

void usage()
{
  auto file = open("foo", "wb");
  write(file, "bar", 3);
  close(file);
}
```

## POSIX close

- invalid value is not always 0 or `nullptr`
- shows why choice of `close_handle`: there are all sort of `close` functions already,
  less tricks required to ensure we call the right one

```cpp
// e.g. in a posixlib namespace:
struct file_handle_traits {
  using handle_type = int;
  static constexpr auto invalid_value() noexcept { return -1; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(close(h));
  }
};
using file_handle = cpp_util::unique_handle<file_handle_traits>;
```

## Windows typical handle

- the `handle_type` is opaque, unlike the previous example where it's a `FILE *`.

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

- `INVALID_HANDLE_VALUE` uses `reinterpret_cast`, so `invalid_handle` is not `constexpr`.
- Often the underlying `handle_type` is reused by the underlying C API with different meanings.
  The traits approach creates different types to disambiguate betweeen those meanings when
  desired.

```cpp
// e.g. also in a winlib namespace:
struct file_handle_traits {
  using handle_type = HANDLE;
  static auto invalid_value() noexcept { return INVALID_HANDLE_VALUE; }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(::CloseHandle(h));
  }
};
using file_handle = cpp_util::unique_handle<file_handle_traits>;
```

## Windows any handle

- `is_valid` allows multiple "invalid" values
- This approach is useful to avoid to disambiguate, if we're convinced that there are truly
  multiple "invalid" values (e.g. that a C API that returns `NULL` will not return
  `INVALID_HANDLE_VALUE` as a valid handle)

```cpp
// e.g. also in a winlib namespace:
struct windows_any_handle_traits {
  using handle_type = HANDLE;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static constexpr auto is_valid(handle_type h) noexcept {
    return (h != nullptr) || (h != INVALID_HANDLE_VALUE);
  }
  static void close_handle(handle_type h) noexcept {
    static_cast<void>(::CloseHandle(h));
  }
};
using windows_any_handle = cpp_util::unique_handle<windows_any_handle_traits>;
```

## Windows device context associated to (GUI) window

- Custom `handle_type` that has muliple values required for `close_handle`
- Shows the option to use `const handle_type & h` for `is_valid` and `close_handle`
- The constructor of `unique_handle` that accepts multiple arguments is
  useful in this scenario

```cpp
struct window_dc_handle_traits {
  struct handle_type {
    HWND hwnd;
    HDC hdc;
  };
  static auto invalid_value() noexcept {
    return handle_type{ NULL, NULL };
  }
  static constexpr auto is_valid(const handle_type & h) noexcept {
    return h.hdc != NULL;
  }
  static void close_handle(const handle_type & h) noexcept {
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
struct scoped_coroutine_handle_traits
{
  using handle_type = std::coroutine_handle<Promise>;
  static constexpr auto invalid_value() noexcept { return nullptr; }
  static constexpr auto is_valid(handle_type h) noexcept { return h.operator bool(); }
  static void close_handle(handle_type h) noexcept { h.destroy(); }
};
template<typename Promise>
using scoped_coroutine_handle = cpp_util::unique_handle<scoped_coroutine_handle_traits<Promise>>;
```

# Traits allow for options

- `SC_HANDLE` is another example of a ambiguous usage in a C API: it could be a service, or it
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
