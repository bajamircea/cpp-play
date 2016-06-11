#include "file_contents.h"

#include "numeric.h"

#include <fstream>
#include <stdexcept>

namespace file_contents
{
  std::string read(const std::string & file_name)
  {
    std::ifstream file{ file_name, std::ios::binary };
    if (!file)
    {
      throw std::runtime_error("Failed to open file for read");
    }

    std::string contents;

    std::streampos begin = file.tellg();
    if (begin < 0)
    {
      throw std::runtime_error("Failed to get file begin position");
    }

    if (!file.seekg(0, std::ios::end))
    {
      throw std::runtime_error("Failed to get to end of file position");
    }

    std::streampos end = file.tellg();
    if (end < 0)
    {
      throw std::runtime_error("Failed to get file end position");
    }

    std::streamsize size = end - begin;
    if (size == 0)
    {
      return contents;
    }

    if (!file.seekg(0, std::ios::beg))
    {
      throw std::runtime_error("Failed to get to beginning of file position");
    }

    contents.resize(numeric::cast<decltype(contents.size())>(size));

    if (!file.read(&contents[0], size))
    {
      throw std::runtime_error("Failed to read file contents");
    }

    file.close();
    if (!file)
    {
      throw std::runtime_error("Failed to close file");
    }

    return contents;
  }

  void write(const std::string & file_name, const std::string & contents)
  {
    std::ofstream file{ file_name, std::ios::binary | std::ios::trunc };
    if (!file)
    {
      throw std::runtime_error("Failed to open file for write");
    }

    if (! contents.empty())
    {
      if (!file.write(&contents[0], numeric::cast<std::streamsize>(contents.size())))
      {
        throw std::runtime_error("Failed to write file contents");
      }
    }

    file.close();
    if (!file)
    {
      throw std::runtime_error("Failed to close file");
    }
  }
} // namespace file_contents
