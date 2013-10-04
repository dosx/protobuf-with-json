// Author: Walt Lin
// Protobuf compiler to Java services.

#include <stdio.h>
#include <assert.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>

#include "util.h"
#include "java_helper.h"
#include "options.pb.h"  // for method options

using namespace google::protobuf;
using namespace google::protobuf::compiler;

// Given a field type, return the corresponding JSON type (for json.getFoo).
static string GetJsonType(const FieldDescriptor* d) {
  switch (d->cpp_type()) {
    case FieldDescriptor::CPPTYPE_DOUBLE:
    case FieldDescriptor::CPPTYPE_FLOAT:
      return "Double";
    case FieldDescriptor::CPPTYPE_INT64:
    case FieldDescriptor::CPPTYPE_UINT64:
      return "Long";
    case FieldDescriptor::CPPTYPE_INT32:
    case FieldDescriptor::CPPTYPE_UINT32:
      return "Int";
    case FieldDescriptor::CPPTYPE_BOOL:
      return "Boolean";
    case FieldDescriptor::CPPTYPE_ENUM:
      return "Int";
    case FieldDescriptor::CPPTYPE_STRING:
      return "String";
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return "JSONObject";
    default:
      return "UNKNOWN";
  }
}

// Get a java type for the field.
static string GetJavaType(const FieldDescriptor* d) {
  switch (d->cpp_type()) {
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return "double";
    case FieldDescriptor::CPPTYPE_FLOAT:
      return "float";
    case FieldDescriptor::CPPTYPE_INT64:
    case FieldDescriptor::CPPTYPE_UINT64:
      return "long";
    case FieldDescriptor::CPPTYPE_INT32:
    case FieldDescriptor::CPPTYPE_UINT32:
      return "int";
    case FieldDescriptor::CPPTYPE_BOOL:
      return "boolean";
    case FieldDescriptor::CPPTYPE_STRING:
      return "String";
    case FieldDescriptor::CPPTYPE_ENUM:
      return java::ClassName(d->enum_type());
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return java::ClassName(d->message_type());
    default:
      return "UNKNOWN";
  }
}

// Get a boxed java type for the field.
static string GetBoxedJavaType(const FieldDescriptor* d) {
  switch (d->cpp_type()) {
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return "Double";
    case FieldDescriptor::CPPTYPE_FLOAT:
      return "Float";
    case FieldDescriptor::CPPTYPE_INT64:
    case FieldDescriptor::CPPTYPE_UINT64:
      return "Long";
    case FieldDescriptor::CPPTYPE_INT32:
    case FieldDescriptor::CPPTYPE_UINT32:
      return "Integer";
    case FieldDescriptor::CPPTYPE_BOOL:
      return "Boolean";
    case FieldDescriptor::CPPTYPE_STRING:
      return "String";
    case FieldDescriptor::CPPTYPE_ENUM:
      return "Integer";  // TODO(walt): is this right?
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return java::ClassName(d->message_type());
    default:
      return "UNKNOWN";
  }
}

// Given a java expresion and its type, return a string that is the boxed value.
static string BoxJavaValue(const string& expr, const FieldDescriptor* d) {
  switch (d->cpp_type()) {
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return "Double.valueOf(" + expr + ")";
    case FieldDescriptor::CPPTYPE_FLOAT:
      return "Float.valueOf(" + expr + ")";
    case FieldDescriptor::CPPTYPE_INT64:
    case FieldDescriptor::CPPTYPE_UINT64:
      return "Long.valueOf(" + expr + ")";
    case FieldDescriptor::CPPTYPE_INT32:
    case FieldDescriptor::CPPTYPE_UINT32:
      return "Integer.valueOf(" + expr + ")";
    case FieldDescriptor::CPPTYPE_BOOL:
      return "Boolean.valueOf(" + expr + ")";
    case FieldDescriptor::CPPTYPE_STRING:
      return expr;
    case FieldDescriptor::CPPTYPE_ENUM:
      return "Integer.valueOf(" + expr + ".getNumber())";
    default:
      return expr;
  }
}

// Given a java expression and its type, return code for converting to a string.
static string JavaValueToString(const string& expr, const FieldDescriptor* d) {
  return BoxJavaValue(expr, d) + ".toString()";
}

// Sigh; we use "_id" as a field name, and we don't want to turn that into "Id".
static string MyUnderscoresToCamelCase(const FieldDescriptor* fd) {
  return fd->name() == "_id" ? "_id" : java::UnderscoresToCamelCase(fd);
}

class FieldGenerator {
 public:
  FieldGenerator(const FieldDescriptor* descriptor, string* error)
      : descriptor_(descriptor), error_(error), is_map_(false),
        map_key_(NULL), map_val_(NULL) {
    vars_["field"] = MyUnderscoresToCamelCase(descriptor);
    vars_["upperfield"] = java::UnderscoresToCapitalizedCamelCase(descriptor);
    vars_["jsontype"] = GetJsonType(descriptor);
    vars_["javatype"] = GetJavaType(descriptor);

    const FieldOptions& opts = descriptor->options();
    string map_key = opts.GetExtension(dx_map_key);
    string map_val = opts.GetExtension(dx_map_val);
    is_map_ = !map_key.empty();
    if (is_map_) {
      const Descriptor* msg = descriptor_->message_type();
      map_key_ = msg->FindFieldByName(map_key);
      map_val_ = msg->FindFieldByName(map_val);
      if (map_key_ == NULL || map_val_ == NULL) {
        error->assign("couldn't look up key or val");
      } else {
        vars_["key_field"] = java::UnderscoresToCapitalizedCamelCase(map_key_);
        vars_["val_field"] = java::UnderscoresToCapitalizedCamelCase(map_val_);
        vars_["val_type"] = GetJsonType(map_val_);
        vars_["val_java_type"] = GetBoxedJavaType(map_val_);
      }
    }
  }

  void GenerateParseJson(io::Printer* printer) {
    if (is_map_) {
      // This is a map. Decode into an array of items.
      // Note: we assume the key is a string.
      // TODO(walt): handle dynamic key -> array.
      printer->Print(vars_,
          "if (json.has(\"$field$\")) {\n"
          "  org.json.JSONObject obj = json.getJSONObject(\"$field$\");\n"
          "  java.util.Iterator<String> keys = obj.keys();\n"
          "  while (keys.hasNext()) {\n"
          "    $javatype$.Builder item = $javatype$.newBuilder();\n"
          "    String key = keys.next();\n"
          "    item.set$key_field$(key);\n"
                     );
      // TODO(walt): we don't handle repeated here yet.
      if (map_val_->type() == FieldDescriptor::TYPE_MESSAGE) {
        printer->Print(vars_,
          "    item.set$val_field$($val_java_type$.parseFromJSON(obj.getJSONObject(key)));\n");
      } else {
        printer->Print(vars_,
          "    item.set$val_field$(obj.get$val_type$(key));\n");
      }
      printer->Print(vars_,
          "    builder.add$upperfield$(item.build());\n"
          "  }\n"
          "}\n"
          "");

    } else if (descriptor_->is_repeated()) {
      printer->Print(vars_,
          "if (json.has(\"$field$\")) {\n"
          "  org.json.JSONArray arr = json.getJSONArray(\"$field$\");\n"
          "  for (int i = 0; i < arr.length(); i++) {\n");
      if (descriptor_->type() == FieldDescriptor::TYPE_MESSAGE) {
        printer->Print(vars_,
            "    $javatype$ parsed = $javatype$.parseFromJSON(arr.getJSONObject(i));\n"
            "    builder.add$upperfield$(parsed);\n");
      } else {
        printer->Print(vars_,
            "    builder.add$upperfield$(arr.get$jsontype$(i));\n");
      }
      printer->Print(
          "  }\n"
          "}\n");

    } else if (descriptor_->type() == FieldDescriptor::TYPE_MESSAGE) {
      printer->Print(vars_,
          "if (json.has(\"$field$\")) {\n"
          "  $javatype$ parsed = $javatype$.parseFromJSON(json.get$jsontype$(\"$field$\"));\n"
          "  builder.set$upperfield$(parsed);\n"
          "}\n");

    } else if (descriptor_->type() == FieldDescriptor::TYPE_ENUM) {
      printer->Print(vars_,
          "if (json.has(\"$field$\") && !json.isNull(\"$field$\")) {\n"
          "  $javatype$ parsed = $javatype$.valueOf(json.get$jsontype$(\"$field$\"));\n"
          "  if (parsed != null) {\n"
          "    builder.set$upperfield$(parsed);\n"
          "  }\n"
          "}\n");

    } else {
      // Primitive type.
      if (descriptor_->type() == FieldDescriptor::TYPE_FLOAT) {
        // JSONObject doesn't have a getFloat, so get a double and cast to float
        printer->Print(vars_,
            "if (json.has(\"$field$\") && !json.isNull(\"$field$\")) {\n"
            "  builder.set$upperfield$(($javatype$)json.get$jsontype$(\"$field$\"));\n"
            "}\n");
      } else {
        printer->Print(vars_,
            "if (json.has(\"$field$\") && !json.isNull(\"$field$\")) {\n"
            "  builder.set$upperfield$(json.get$jsontype$(\"$field$\"));\n"
            "}\n");
      }
    }
  }

  void GenerateToJson(io::Printer* printer) {
    if (is_map_) {
      printer->Print(vars_,
          "if (get$upperfield$Count() > 0) {\n"
          "  org.json.JSONObject obj = new org.json.JSONObject();\n"
          "  for (int i = 0; i < get$upperfield$Count(); i++) {\n"
          "    $javatype$ el = get$upperfield$(i);\n"
          "    String key = el.get$key_field$();\n");
      // TODO(walt): we don't handle repeated here yet.
      if (map_val_->type() == FieldDescriptor::TYPE_MESSAGE) {
        printer->Print(vars_,
          "    obj.put(key, el.get$val_field$().toJSON());\n");
      } else {
        printer->Print(vars_,
          "    obj.put(key, el.get$val_field$());\n");
      }
      printer->Print(
          "  }\n"
          "}\n");

    } else if (descriptor_->is_repeated()) {
      printer->Print(vars_,
          "if (get$upperfield$Count() > 0) {\n"
          "  org.json.JSONArray arr = new org.json.JSONArray();\n"
          "  for (int i = 0; i < get$upperfield$Count(); i++) {\n"
          "    $javatype$ el = get$upperfield$(i);\n");
      if (descriptor_->type() == FieldDescriptor::TYPE_MESSAGE) {
        printer->Print("    arr.put(el.toJSON());\n");
      } else {
        printer->Print("    arr.put(el);\n");
      }
      printer->Print(vars_,
          "  }\n"
          "  json.put(\"$field$\", arr);\n"
          "}\n");

    } else if (descriptor_->type() == FieldDescriptor::TYPE_MESSAGE) {
      printer->Print(vars_,
          "if (has$upperfield$()) {\n"
          "  json.put(\"$field$\", get$upperfield$().toJSON());\n"
          "}\n");

    } else if (descriptor_->type() == FieldDescriptor::TYPE_ENUM) {
      printer->Print(vars_,
          "if (has$upperfield$()) {\n"
          "  json.put(\"$field$\", get$upperfield$().getNumber());\n"
          "}\n");
    } else {
      // Primitive type.
      printer->Print(vars_,
          "if (has$upperfield$()) {\n"
          "  json.put(\"$field$\", get$upperfield$());\n"
          "}\n");
    }
  }

  // Since this field is a map, generate getMap and getKeys methods.
  // TODO(walt): use iterable for getKeys?
  void GenerateGetMap(io::Printer* printer) {
    if (!is_map_) {
      return;
    }

    printer->Print(vars_,
        "public java.util.Map<String, $val_java_type$> get$upperfield$AsMap() {\n"
        "  java.util.Map<String, $val_java_type$> map = new java.util.HashMap<String, $val_java_type$>();\n"
        "  for ($javatype$ x : this.get$upperfield$List()) {\n"
        "    map.put(x.get$key_field$(), x.get$val_field$());\n"
        "  }\n"
        "  return map;\n"
        "}\n"
        "\n"
        "public java.util.List<String> get$upperfield$Keys() {\n"
        "  java.util.List<String> list = new java.util.ArrayList<String>();\n"
        "  for ($javatype$ x : this.get$upperfield$List()) {\n"
        "    list.add(x.get$key_field$());\n"
        "  }\n"
        "  return list;\n"
        "}\n"
        "\n"
        "public boolean contains$upperfield$Key(String key) {\n"
        "  for ($javatype$ x : this.get$upperfield$List()) {\n"
        "    if (key.equals(x.get$key_field$())) {\n"
        "      return true;\n"
        "    }\n"
        "  }\n"
        "  return false;\n"
        "}\n"
        "\n"
        "public $val_java_type$ get$upperfield$Value(String key) {\n"
        "  for ($javatype$ x : this.get$upperfield$List()) {\n"
        "    if (key.equals(x.get$key_field$())) {\n"
        "      return x.get$val_field$();\n"
        "    }\n"
        "  }\n"
        "  return null;\n"
        "}\n"
        "\n"
        "");
  }

  static bool SupportsToMap(const FieldDescriptor *d) {
    return !d->is_repeated() && d->type() != FieldDescriptor::TYPE_MESSAGE;
  }

  void GenerateToMap(io::Printer* printer) {
    if (descriptor_->is_repeated() ||
        descriptor_->type() == FieldDescriptor::TYPE_MESSAGE) {
      error_->assign("type not supported for ToMap");
      return;
    }

    // Primitive type.
    string out =
        "if (has$upperfield$()) {\n"
        "  m.put(\"$field$\", " +
        JavaValueToString("get$upperfield$()", descriptor_) +
        ");\n"
        "}\n";
    printer->Print(vars_, out.c_str());
  }

 private:
  const FieldDescriptor* descriptor_;
  string* error_;
  map<string, string> vars_;
  bool is_map_;
  const FieldDescriptor* map_key_;
  const FieldDescriptor* map_val_;
};

// Generate parseFrom method on a message.
class MessageGenerator {
 public:
  MessageGenerator(const Descriptor* descriptor,
                   string* error)
      : descriptor_(descriptor), error_(error) {
    vars_["classname"] = java::ClassName(descriptor_);
  }

  void GenerateSource(io::Printer* printer) {
    GenerateGetMap(printer);
    printer->Print("\n");
    GenerateParseJson(printer);
    printer->Print("\n");
    GenerateToJson(printer);
    printer->Print("\n");
    GenerateToMap(printer);
    printer->Print("\n");
  }

 private:
  // parseFromJSON method.
  void GenerateParseJson(io::Printer* printer) {
    printer->Print(vars_,
        "public static $classname$ parseFromJSON("
        "org.json.JSONObject json) throws org.json.JSONException {\n");
    printer->Indent();
    printer->Print(vars_,
        "$classname$.Builder builder = $classname$.newBuilder();\n");
    for (int i = 0; i < descriptor_->field_count(); i++) {
      FieldGenerator(descriptor_->field(i), error_).GenerateParseJson(printer);
    }
    printer->Print("return builder.build();\n");
    printer->Outdent();
    printer->Print("}\n");
  }

  // toJSON method.
  void GenerateToJson(io::Printer* printer) {
    printer->Print(
        "public org.json.JSONObject toJSON() throws org.json.JSONException {\n"
        "  org.json.JSONObject json = new org.json.JSONObject();\n");
    printer->Indent();
    for (int i = 0; i < descriptor_->field_count(); i++) {
      FieldGenerator(descriptor_->field(i), error_).GenerateToJson(printer);
    }
    printer->Print("return json;\n");
    printer->Outdent();
    printer->Print("}\n");
  }

  // toMap method.
  void GenerateToMap(io::Printer* printer) {
    bool supported = true;
    for (int i = 0; i < descriptor_->field_count(); i++) {
      supported &= FieldGenerator::SupportsToMap(descriptor_->field(i));
    }

    if (supported) {
      printer->Print("public java.util.Map<String,?> toMap() {\n");
      printer->Indent();
      printer->Print(vars_,
          "java.util.Map<String, String> m = "
          "new java.util.HashMap<String, String>();\n");
      for (int i = 0; i < descriptor_->field_count(); i++) {
        FieldGenerator(descriptor_->field(i), error_).GenerateToMap(printer);
      }
      printer->Print("return m;\n");
      printer->Outdent();
      printer->Print("}\n");
    }
  }

  // getMap method for fields that are maps.
  void GenerateGetMap(io::Printer* printer) {
    for (int i = 0; i < descriptor_->field_count(); i++) {
      FieldGenerator(descriptor_->field(i), error_).GenerateGetMap(printer);
    }
  }

  const Descriptor* descriptor_;
  string* error_;
  map<string, string> vars_;
};


// Generate one method on a service.
class MethodGenerator {
 public:
  MethodGenerator(const MethodDescriptor* descriptor, string* error)
      : descriptor_(descriptor), error_(error) {
    DXMethodOptions options =
        descriptor_->options().GetExtension(dx_method_options);
    vars_["method_name"] = descriptor->name();
    vars_["input_class"] = java::ClassName(descriptor->input_type());
    vars_["output_class"] = java::ClassName(descriptor->output_type());
    vars_["http_method"] = options.http_method();
  }

  void GenerateSource(io::Printer* printer) {
    if (!descriptor_->options().HasExtension(dx_method_options)) {
      error_->assign("Error: can't generate method %s, "
                     "doesn't have options set");
      return;
    }

    //fprintf(stderr, "doing method %s\n", descriptor_->DebugString().c_str());
    DXMethodOptions options =
        descriptor_->options().GetExtension(dx_method_options);

    // Pull out all args from the path, we'll use them as the first arguments to
    // this method. The args are specified like express.js.
    string path = options.path();
    vector<string> path_parts;
    vector<string> method_args;
    while (!path.empty()) {
      int i = path.find(':');
      int j = path.find('/', i + 1);
      path_parts.push_back(path.substr(0, i));
      if (i == string::npos) {
        break;
      } else if (j == string::npos) {
        method_args.push_back(path.substr(i + 1));
        break;
      } else {
        method_args.push_back(path.substr(i + 1, j - i - 1));
        path = path.substr(j);
      }
    }

    printer->Print(vars_, "public void $method_name$(");
    for (int i = 0; i < method_args.size(); i++) {
      printer->Print("String $v$, ", "v", method_args[i]);
    }

    printer->Print(vars_, "$input_class$ req, "
                   "final Callback<$output_class$> callback) {\n");
    printer->Indent();

    const char* sep = "";
    printer->Print("String path = ");
    for (int i = 0; i < method_args.size(); i++) {
      printer->Print(sep);
      printer->Print("\"$pp$\"", "pp", path_parts[i]);
      printer->Print(" + $v$", "v", method_args[i]);
      sep = " + ";
    }
    if (path_parts.size() > method_args.size()) {
      printer->Print(sep);
      printer->Print("\"$pp$\";\n", "pp", path_parts.back());
    } else {
      printer->Print(";\n");
    }

    printer->Print(
        vars_,
        "org.json.JSONObject params = null;\n"
        "try {\n"
        "  params = req.toJSON();\n"
        "} catch (org.json.JSONException e) {\n"
        "  callback.done(-1, \"JSON error: \" + e, null);\n"
        "  return;\n"
        "}\n"
        "this.doCall(path, \"$http_method$\", params, $output_class$.class, callback);\n"
        "");

    printer->Outdent();
    printer->Print("}\n\n");
  }

 private:
  const MethodDescriptor* descriptor_;
  string* error_;
  map<string, string> vars_;
};


// Generate code for a service.
class ServiceGenerator {
 public:
  ServiceGenerator(const ServiceDescriptor* descriptor, string* error)
      : descriptor_(descriptor), error_(error) {
  }

  void GenerateSource(io::Printer* printer) {
    printer->Print("public static abstract class $name$ {\n",
                   "name", descriptor_->name());
    printer->Indent();
    printer->Print(
        "public static interface Callback<T> {\n"
        "    void done(int code, String error, T response);\n"
        "}\n"
        "\n"
        "protected abstract <T> void doCall(\n"
        "    final String path,\n"
        "    final String httpMethod,\n"
        "    final org.json.JSONObject params,\n"
        "    final Class<T> responseType,\n"
        "    final Callback<T> callback);\n\n");
    for (int i = 0; i < descriptor_->method_count(); i++) {
      MethodGenerator(descriptor_->method(i), error_).GenerateSource(printer);
    }
    printer->Outdent();
    printer->Print("}\n\n");
  }

 private:
  const ServiceDescriptor* descriptor_;
  string* error_;
};


class MyCodeGenerator : public CodeGenerator {
 public:
  virtual ~MyCodeGenerator() {}

  static void doMessage(const Descriptor* d,
                        const string& java_filename,
                        GeneratorContext* context,
                        string* error) {
    {
      scoped_ptr<io::ZeroCopyOutputStream> output(context->OpenForInsert(
          java_filename, "class_scope:" + d->full_name()));
      io::Printer printer(output.get(), '$');
      MessageGenerator(d, error).GenerateSource(&printer);
    }
    for (int j = 0; j < d->nested_type_count(); j++) {
      doMessage(d->nested_type(j), java_filename, context, error);
    }
  }

  virtual bool Generate(const FileDescriptor* file,
                        const string& parameter,
                        GeneratorContext* context,
                        string* error) const {
    string package_dir = java::JavaPackageToDir(java::FileJavaPackage(file));
    string java_filename = package_dir;
    java_filename += java::FileClassName(file);
    java_filename += ".java";

    // Insert methods to parse/generate JSON.
    for (int i = 0; i < file->message_type_count(); i++) {
      doMessage(file->message_type(i), java_filename, context, error);
    }

    // Insert service code.
    {
      scoped_ptr<io::ZeroCopyOutputStream> output(context->OpenForInsert(
          java_filename, "outer_class_scope"));
      io::Printer printer(output.get(), '$');
      for (int i = 0; i < file->service_count(); i++) {
        ServiceGenerator(file->service(i), error).GenerateSource(&printer);
      }
    }

    if (!error->empty()) {
      fprintf(stderr, "ERROR: %s\n", error->c_str());
    }
    return error->empty();
  }
};

int main(int argc, char* argv[]) {
  MyCodeGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
