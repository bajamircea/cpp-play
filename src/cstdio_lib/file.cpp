#include "file.h"

#include "error.h"

namespace cstdio::file
{
  file_handle open(const char * file_name, const char * mode)
  {
    file_handle result{ std::fopen(file_name, mode) };
    if (!result.is_valid())
    {
      error::throw_errno("fopen");
    }
    return result;
  }

  size_t read(file_arg h, char * buffer, size_t size)
  {
    size_t read_count{ std::fread(buffer, 1, size, h) };
    if ((read_count != size) && ferror(h))
    {
      error::throw_errno("fread");
    }
    return read_count;
  }

  void write(file_arg h, const char * buffer, size_t size)
  {
    size_t write_count{ std::fwrite(buffer , 1, size, h) };
    if (write_count != size)
    {
      error::throw_errno("fwrite");
    }
  }

  bool is_eof(file_arg h)
  {
    return (0 != std::feof(h));
  }

  void close(file_handle & x)
  {
    int result = std::fclose(x.release());
    if (result != 0)
    {
      error::throw_failed("fclose");
    }
  }
}