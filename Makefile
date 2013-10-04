# To use the compiler:
#  protoc --plugin=protoc-gen-js=./protoc-gen-js --js_out=.  myfile.proto

PROTOC = protoc
CC = g++
PROTODIR = proto
CFLAGS = -I $(PROTODIR)
LDLIBS = -lprotobuf -lprotoc

OPTIONS_SRC = options.pb.cc

JAVA_TARGET = protoc-gen-jsonjava
JAVA_SOURCES = java_generator.cc util.cc $(OPTIONS_SRC)
JAVA_OBJECTS = $(subst .cc,.o,$(JAVA_SOURCES))

all: $(JAVA_TARGET)

$(JAVA_TARGET): $(JAVA_OBJECTS)
	$(CC) -o $(JAVA_TARGET) $(JAVA_OBJECTS) $(LDLIBS)

$(OPTIONS_SRC): $(PROTODIR)/options.proto
	$(PROTOC) -I $(PROTODIR) --cpp_out=. $(PROTODIR)/options.proto

java_generator.cc: $(OPTIONS_SRC)

example: $(JAVA_TARGET)
	$(PROTOC) -I $(PROTODIR) --plugin=protoc-gen-jsonjava --java_out=. --jsonjava_out=. $(PROTODIR)/example.proto

.PHONY: clean example

clean:
	rm -f *.o options.pb.h options.pb.cc $(JAVA_TARGET)

%.o: %.cc
	$(CC) $(CFLAGS) -c $<
