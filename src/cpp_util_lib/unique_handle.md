# Usage samples for `unique_handle`

## cstdio file handle

Shows simple/typical usage pattern for `unique_handle` and `handle_arg`

```cpp
  struct file_handle_traits {
    using handle_type = FILE *;
    static constexpr auto invalid_value() noexcept { return nullptr; }
    static void close_handle(handle_type h) noexcept {
      static_cast<void>(std::fclose(h));
    }
  };
  using file_handle = cpp_util::unique_handle<file_handle_traits>;
  using file_arg = cpp_util::handle_arg<file_handle>;

  file_handle open(const char * file_name, const char * mode)
  {
    file_handle result{ std::fopen(file_name, mode) };
    if (!result.is_valid())
    {
      error::throw_errno("fopen");
    }
    return result;
  }

  void write(file_arg h, const char * buffer, size_t size)
  {
    size_t write_count{ std::fwrite(buffer , 1, size, h) };
    if (write_count != size)
    {
      error::throw_errno("fwrite");
    }
  }

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

## Windows typical handle

- the `handle_type` is opaque, unlike the previous example where it's a `FILE *`.

```cpp
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

```cpp
  struct windows_file_handle_traits {
    using handle_type = HANDLE;
    static auto invalid_value() noexcept { return INVALID_HANDLE_VALUE; }
    static void close_handle(handle_type h) noexcept {
      static_cast<void>(::CloseHandle(h));
    }
  };
  using windows_file_handle = cpp_util::unique_handle<windows_file_handle_traits>;
```

## Windows any handle

- `is_valid` allows multiple "invalid" values
- also the constructor of `unique_handle` that accepts multiple arguments is
  useful in this scenario

```cpp
  struct windows_any_handle_traits {
    using handle_type = HANDLE;
    static constexpr auto invalid_value() noexcept { return nullptr; }
    static constexpr auto is_valid(const handle_type & h) noexcept {
      return (h != nullptr) && (h != INVALID_HANDLE_VALUE);
    }
    static void close_handle(handle_type h) noexcept {
      static_cast<void>(::CloseHandle(h));
    }
  };
  using windows_any_handle = cpp_util::unique_handle<windows_any_handle_traits>;
```

## Windows device context associated to (GUI) window

- custom `handle_type` that has muliple values required for `close_handle`

```cpp
  struct window_dc_handle_traits {
    struct handle_type {
      HWND hwnd;
      HDC hdc;
    };
    static auto invalid_value() noexcept {
      return handle_type{ nullptr, nullptr };
    }
    static constexpr auto is_valid(const handle_type & h) noexcept {
      return h.hdc != nullptr;
    }
    static void close_handle(const handle_type & h) noexcept {
      static_cast<void>(::ReleaseDC(h.hwnd, h.hdc));
    }
  };
  using window_dc_handle = cpp_util::unique_handle<window_dc_handle_traits>;
```

## Cleanup action to perform
```cpp
  struct revert_to_self_handle_traits {
    using handle_type = bool;
    static constexpr auto invalid_value() noexcept { return false; }
    static void close_handle(bool) noexcept {
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