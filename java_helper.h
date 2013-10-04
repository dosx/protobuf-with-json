// Forward declarations, copied from java_helpers.h in google source.

#ifndef PROTOBUF_FOR_PB_JAVA_HELPER_H__
#define PROTOBUF_FOR_PB_JAVA_HELPER_H__

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

// Converts the field's name to camel-case, e.g. "foo_bar_baz" becomes
// "fooBarBaz" or "FooBarBaz", respectively.
string UnderscoresToCamelCase(const FieldDescriptor* field);
string UnderscoresToCapitalizedCamelCase(const FieldDescriptor* field);

// Similar, but for method names.  (Typically, this merely has the effect
// of lower-casing the first letter of the name.)
string UnderscoresToCamelCase(const MethodDescriptor* method);

// Strips ".proto" or ".protodevel" from the end of a filename.
string StripProto(const string& filename);

// Gets the unqualified class name for the file.  Each .proto file becomes a
// single Java class, with all its contents nested in that class.
string FileClassName(const FileDescriptor* file);

// Returns the file's Java package name.
string FileJavaPackage(const FileDescriptor* file);

// Returns output directory for the given package name.
string JavaPackageToDir(string package_name);

// Converts the given fully-qualified name in the proto namespace to its
// fully-qualified name in the Java namespace, given that it is in the given
// file.
string ToJavaName(const string& full_name, const FileDescriptor* file);

// These return the fully-qualified class name corresponding to the given
// descriptor.
string ClassName(const Descriptor* descriptor);
string ClassName(const EnumDescriptor* descriptor);
string ClassName(const ServiceDescriptor* descriptor);
string ClassName(const FileDescriptor* descriptor);

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // PROTOBUF_FOR_PB_JAVA_HELPER_H__
