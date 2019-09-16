#pragma once

#include <string>
#include <vector>

namespace rhythmus
{

/* @brief Directory item info */
struct DirItem
{
  std::string filename;
  int is_file;
  int64_t timestamp_modified;
};

/* @brief Get all items in a directory (without recursive) */
bool GetDirectoryItems(const std::string& dirpath, std::vector<DirItem>& out);

/* @brief string format util */
template <typename ... Args>
std::string format_string(const std::string& format, Args ... args)
{
  size_t size = snprintf(nullptr, 0, format.c_str(), args ...);
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, format.c_str(), args ...);
  return std::string(buf.get(), buf.get() + size - 1);
}

}