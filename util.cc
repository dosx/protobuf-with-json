#include "util.h"
#include <stdio.h>

namespace google {
namespace protobuf {
namespace compiler {

const char kFileHeader[] =
    "//\n"
    "//  AUTO GENERATED FILE BY PROTO COMPILER, DO NOT EDIT!!!\n"
    "//\n"
    "\n\n";

std::string Basename(const std::string& fn) {
  return fn.substr(0, fn.find_last_of('.'));
}

std::string SimpleItoa(int x) {
  char buf[16];
  sprintf(buf, "%d", x);
  return std::string(buf);
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
