CXX=g++

CXXFLAGS=-g $(shell root-config --cflags) -fmodules-ts -ftemplate-depth=32768 -s -O2 -std=c++20 -fconcepts-diagnostics-depth=5

LDFLAGS=-g $(shell root-config --ldflags)
LDLIBS=-g $(shell root-config --libs) -lfmt

HEADER_FILES = \
	src/type_helpers.hpp

SOURCE_FILES= \
	src/main.cpp
OBJECT_FILES=$(subst .cpp,.cpp.o,$(SOURCE_FILES))

sandbox: $(OBJECT_FILES)
	$(CXX) $(LDFLAGS) -o compile_time_bf $(OBJECT_FILES) $(LDLIBS)

clean:
	rm -f $(OBJECT_FILES)
	rm -f sandbox

%.cpp.o: %.cpp $(HEADER_FILES)
	$(CXX) $(CXXFLAGS) -c $< -o $<.o