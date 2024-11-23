# This Makefile uses code from https://gist.github.com/mihaitodor/bfb8e7ad908489fdf3ceb496573f306a

# Need to use a C++ compiler because GTest is a C++ library
# g++ can compile C code just fine, although there are some minor behavior differences compared to gcc (mostly good differences)
CXX := g++
GTEST_DIR := googletest/googletest

# Pre-processor flags
CPPFLAGS := -isystem $(GTEST_DIR)/include
# Flags passed to C++ compiler
CXXFLAGS := -g -Wall -Werror -pthread

# All test executables produced by this Makefile
# If necessary, add new executables you create to the list
TESTS := tests

# Set Google Test's header directory as a system directory, such that
# the compiler doesn't generate warnings in Google Test headers
GTEST_HEADERS := $(GTEST_DIR)/include/gtest/*.h \
				$(GTEST_DIR)/include/gtest/internal/*.h
GTEST_SRCS_ := $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# https://stackoverflow.com/questions/2145590/what-is-the-purpose-of-phony-in-a-makefile
.PHONY: all test main clean valgrind

all: $(TESTS) main

test: all
	./tests

main.o: main.c shell.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c main.c

main: main.o shell.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@

shell.o: shell.c shell.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c shell.c

tests.o: tests.cpp $(GTEST_HEADERS) *.h *.hpp
	$(CXX) $(CPPFLAGS) -DTEST_MODE $(CXXFLAGS) -c tests.cpp

tests: tests.o shell.o gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@

valgrind: $(TESTS)
	valgrind --error-exitcode=1 --leak-check=full --show-leak-kinds=definite --errors-for-leak-kinds=definite ./tests >/dev/null

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized. This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o: $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o: $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c $(GTEST_DIR)/src/gtest_main.cc

gtest.a: gtest-all.o
	ar rcs $@ $^

gtest_main.a: gtest-all.o gtest_main.o
	ar rcs $@ $^

clean:
	rm -f $(TESTS) gtest.a gtest_main.a *.o *.out main test_detail.json
