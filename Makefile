CXX := g++
XMP_BASE := ../XMP-Toolkit-SDK/public
XMP_LIB := $(XMP_BASE)/libraries/i80386linux_x64/release
XMP_INC := $(XMP_BASE)/include
CXXFLAGS := -g -O2 -I$(XMP_INC) -DUNIX_ENV=1 -Wall
LDFLAGS := $(XMP_LIB)/staticXMPCore.ar $(XMP_LIB)/staticXMPFiles.ar -ldl

.PHONY: clean all

all: parser

clean:
	rm -f *.o tool

parser: fhmwg1parse.o fhmwg1ds.o parser.o
	$(CXX) $^ -o parser $(LDFLAGS)

%.o: %.cpp %.hpp
	$(CXX) $(CXXFLAGS) $< -c

