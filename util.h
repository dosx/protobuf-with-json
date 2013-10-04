// Author: Walt Lin
// Helpers.

#ifndef PROTOBUF_FOR_PB_UTIL_H__
#define PROTOBUF_FOR_PB_UTIL_H__

#include <string>

namespace google {
namespace protobuf {
namespace compiler {

std::string Basename(const std::string& fn);

std::string SimpleItoa(int x);

extern const char kFileHeader[];

}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // PROTOBUF_FOR_PB_UTIL_H__
