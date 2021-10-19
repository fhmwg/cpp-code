CXX := g++
XMP_BASE := ../XMP-Toolkit-SDK/public
XMP_LIB := $(XMP_BASE)/libraries/i80386linux_x64/release
XMP_INC := $(XMP_BASE)/include
CXXFLAGS := -g -O2 -I$(XMP_INC) -DUNIX_ENV=1 -Wall -funsigned-char
LDFLAGS := $(XMP_LIB)/staticXMPCore.ar $(XMP_LIB)/staticXMPFiles.ar -ldl

.PHONY: clean all

all: parser writer

clean:
	rm -f *.o tool

parser: fhmwg1parse.o fhmwg1ds.o parser.o
	$(CXX) $^ -o parser $(LDFLAGS)

writer: fhmwg1parse.o fhmwg1ds.o writer.o
	$(CXX) $^ -o writer $(LDFLAGS)

%: %.o
	$(CXX) $^ -o $@ $(LDFLAGS)


%.o: %.cpp %.hpp
	$(CXX) $(CXXFLAGS) $< -c

