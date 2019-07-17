#pragma once
#include <boost/filesystem.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
namespace pixiu {

constexpr struct path {
  using fspath = boost::filesystem::path;
  fspath install() const {
    return boost::filesystem::absolute(
      bin() / ".."
    ).make_preferred();
  }
  fspath bin() const {
    return boost::filesystem::absolute(
      boost::dll::program_location().parent_path()
    ).make_preferred();
  }
 fspath test_data() const {
   return install() / "test_data" / "pixiu";
 } 
} path;

} // namespace pixiu