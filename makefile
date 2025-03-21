# Delete the default suffixes (otherwise visible in 'make -d')
.SUFFIXES:

# Cancel source control implicit rules (otherwise visible in 'make -d')
%: %,v
%: RCS/%,v
%: RCS/%
%: s.%
%: SCCS/s.%

# Folders used
## source files expected in subfolders of SRC_DIR
SRC_DIR = src

## contains executables we build e.g. bin/debug/prog
BIN_DIR = bin
## intermediate build folder e.g. for object files and dependency files e.g. int/debug/prog/main.o
INT_DIR = int

## temp folder e.g. for automated tests
TMP_DIR = tmp

# Compiler flags
CXX = g++
## -MMD creates dependency list, but ignores system includes
## -MF specifies where to create the dependency file name
## -MP creates phony targets for headers (deals with deleted headers after
##  obj file has been compiled)
## -MT specifies the dependency target (path qualified obj file name)
DEP_FLAGS = -MT $@ -MMD -MP -MF $(@:.o=.d)
STD_FLAGS = --std=c++23 -fno-rtti
WARN_FLAGS = -Wall -Wpedantic -Wextra -Werror
debug_FLAGS = -g -D_DEBUG=1 -fsanitize=address
release_FLAGS = -O3 -march=native
CXXFLAGS = $(STD_FLAGS) $(DEP_FLAGS) $(WARN_FLAGS)
LDFLAGS = $(STD_FLAGS) $(WARN_FLAGS)

## Compilation rule for .o files has a dependency on .d file to ensure that if
## the .d file is deleted, the object file is created again e.g. in case a header is changed

# Rules on how to build
## To build all 'make'
.DEFAULT: all

.PHONY: all debug release clean

all : debug release

debug release :

DEP_FILES :=

# Rules for clrs_lib_test

clrs_lib_test_CPP_FILES := $(wildcard $(SRC_DIR)/clrs_lib_test/*.cpp)

debug_clrs_lib_test_OBJ_FILES := $(clrs_lib_test_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/debug/%.o)

$(debug_clrs_lib_test_OBJ_FILES) : $(INT_DIR)/debug/clrs_lib_test/%.o : $(SRC_DIR)/clrs_lib_test/%.cpp $(INT_DIR)/debug/clrs_lib_test/%.d | $(INT_DIR)/debug/clrs_lib_test
	$(CXX) $(CXXFLAGS) $(debug_FLAGS) -c -o $@ $<

$(BIN_DIR)/debug/test/clrs_lib_test : $(debug_clrs_lib_test_OBJ_FILES) $(INT_DIR)/debug/test_lib.a $(INT_DIR)/debug/test_main_lib.a | $(BIN_DIR)/debug/test
	$(CXX) $(LDFLAGS) $(debug_FLAGS) -o $@ $^

$(INT_DIR)/debug/clrs_lib_test/success.run : $(BIN_DIR)/debug/test/clrs_lib_test | $(INT_DIR)/debug/clrs_lib_test
	$^
	touch $@

debug : $(INT_DIR)/debug/clrs_lib_test/success.run

DEP_FILES += $(debug_clrs_lib_test_OBJ_FILES:.o=.d)

release_clrs_lib_test_OBJ_FILES := $(clrs_lib_test_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/release/%.o)

$(release_clrs_lib_test_OBJ_FILES) : $(INT_DIR)/release/clrs_lib_test/%.o : $(SRC_DIR)/clrs_lib_test/%.cpp $(INT_DIR)/release/clrs_lib_test/%.d | $(INT_DIR)/release/clrs_lib_test
	$(CXX) $(CXXFLAGS) $(release_FLAGS) -c -o $@ $<

$(BIN_DIR)/release/test/clrs_lib_test : $(release_clrs_lib_test_OBJ_FILES) $(INT_DIR)/release/test_lib.a $(INT_DIR)/release/test_main_lib.a | $(BIN_DIR)/release/test
	$(CXX) $(LDFLAGS) $(release_FLAGS) -o $@ $^

$(INT_DIR)/release/clrs_lib_test/success.run : $(BIN_DIR)/release/test/clrs_lib_test | $(INT_DIR)/release/clrs_lib_test
	$^
	touch $@

release : $(INT_DIR)/release/clrs_lib_test/success.run

DEP_FILES += $(release_clrs_lib_test_OBJ_FILES:.o=.d)

# Rules for coro_lib_perf

coro_lib_perf_CPP_FILES := $(wildcard $(SRC_DIR)/coro_lib_perf/*.cpp)

debug_coro_lib_perf_OBJ_FILES := $(coro_lib_perf_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/debug/%.o)

$(debug_coro_lib_perf_OBJ_FILES) : $(INT_DIR)/debug/coro_lib_perf/%.o : $(SRC_DIR)/coro_lib_perf/%.cpp $(INT_DIR)/debug/coro_lib_perf/%.d | $(INT_DIR)/debug/coro_lib_perf
	$(CXX) $(CXXFLAGS) $(debug_FLAGS) -c -o $@ $<

$(BIN_DIR)/debug/coro_lib_perf : $(debug_coro_lib_perf_OBJ_FILES) $(INT_DIR)/debug/test_lib.a $(INT_DIR)/debug/test_main_lib.a | $(BIN_DIR)/debug
	$(CXX) $(LDFLAGS) $(debug_FLAGS) -o $@ $^

debug : $(BIN_DIR)/debug/coro_lib_perf

DEP_FILES += $(debug_coro_lib_perf_OBJ_FILES:.o=.d)

release_coro_lib_perf_OBJ_FILES := $(coro_lib_perf_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/release/%.o)

$(release_coro_lib_perf_OBJ_FILES) : $(INT_DIR)/release/coro_lib_perf/%.o : $(SRC_DIR)/coro_lib_perf/%.cpp $(INT_DIR)/release/coro_lib_perf/%.d | $(INT_DIR)/release/coro_lib_perf
	$(CXX) $(CXXFLAGS) $(release_FLAGS) -c -o $@ $<

$(BIN_DIR)/release/coro_lib_perf : $(release_coro_lib_perf_OBJ_FILES) $(INT_DIR)/release/test_lib.a $(INT_DIR)/release/test_main_lib.a | $(BIN_DIR)/release
	$(CXX) $(LDFLAGS) $(release_FLAGS) -o $@ $^

release : $(BIN_DIR)/release/coro_lib_perf

DEP_FILES += $(release_coro_lib_perf_OBJ_FILES:.o=.d)

# Rules for coro_lib_test

coro_lib_test_CPP_FILES := $(wildcard $(SRC_DIR)/coro_lib_test/*.cpp)

debug_coro_lib_test_OBJ_FILES := $(coro_lib_test_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/debug/%.o)

$(debug_coro_lib_test_OBJ_FILES) : $(INT_DIR)/debug/coro_lib_test/%.o : $(SRC_DIR)/coro_lib_test/%.cpp $(INT_DIR)/debug/coro_lib_test/%.d | $(INT_DIR)/debug/coro_lib_test
	$(CXX) $(CXXFLAGS) $(debug_FLAGS) -c -o $@ $<

$(BIN_DIR)/debug/test/coro_lib_test : $(debug_coro_lib_test_OBJ_FILES) $(INT_DIR)/debug/test_lib.a $(INT_DIR)/debug/test_main_lib.a | $(BIN_DIR)/debug/test
	$(CXX) $(LDFLAGS) $(debug_FLAGS) -o $@ $^

$(INT_DIR)/debug/coro_lib_test/success.run : $(BIN_DIR)/debug/test/coro_lib_test | $(INT_DIR)/debug/coro_lib_test
	$^
	touch $@

debug : $(INT_DIR)/debug/coro_lib_test/success.run

DEP_FILES += $(debug_coro_lib_test_OBJ_FILES:.o=.d)

release_coro_lib_test_OBJ_FILES := $(coro_lib_test_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/release/%.o)

$(release_coro_lib_test_OBJ_FILES) : $(INT_DIR)/release/coro_lib_test/%.o : $(SRC_DIR)/coro_lib_test/%.cpp $(INT_DIR)/release/coro_lib_test/%.d | $(INT_DIR)/release/coro_lib_test
	$(CXX) $(CXXFLAGS) $(release_FLAGS) -c -o $@ $<

$(BIN_DIR)/release/test/coro_lib_test : $(release_coro_lib_test_OBJ_FILES) $(INT_DIR)/release/test_lib.a $(INT_DIR)/release/test_main_lib.a | $(BIN_DIR)/release/test
	$(CXX) $(LDFLAGS) $(release_FLAGS) -o $@ $^

$(INT_DIR)/release/coro_lib_test/success.run : $(BIN_DIR)/release/test/coro_lib_test | $(INT_DIR)/release/coro_lib_test
	$^
	touch $@

release : $(INT_DIR)/release/coro_lib_test/success.run

DEP_FILES += $(release_coro_lib_test_OBJ_FILES:.o=.d)

# Rules for cpp_util_lib_test

cpp_util_lib_test_CPP_FILES := $(wildcard $(SRC_DIR)/cpp_util_lib_test/*.cpp)

debug_cpp_util_lib_test_OBJ_FILES := $(cpp_util_lib_test_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/debug/%.o)

$(debug_cpp_util_lib_test_OBJ_FILES) : $(INT_DIR)/debug/cpp_util_lib_test/%.o : $(SRC_DIR)/cpp_util_lib_test/%.cpp $(INT_DIR)/debug/cpp_util_lib_test/%.d | $(INT_DIR)/debug/cpp_util_lib_test
	$(CXX) $(CXXFLAGS) $(debug_FLAGS) -c -o $@ $<

$(BIN_DIR)/debug/test/cpp_util_lib_test : $(debug_cpp_util_lib_test_OBJ_FILES) $(INT_DIR)/debug/test_lib.a $(INT_DIR)/debug/test_main_lib.a | $(BIN_DIR)/debug/test
	$(CXX) $(LDFLAGS) $(debug_FLAGS) -o $@ $^

$(INT_DIR)/debug/cpp_util_lib_test/success.run : $(BIN_DIR)/debug/test/cpp_util_lib_test | $(INT_DIR)/debug/cpp_util_lib_test
	$^
	touch $@

debug : $(INT_DIR)/debug/cpp_util_lib_test/success.run

DEP_FILES += $(debug_cpp_util_lib_test_OBJ_FILES:.o=.d)

release_cpp_util_lib_test_OBJ_FILES := $(cpp_util_lib_test_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/release/%.o)

$(release_cpp_util_lib_test_OBJ_FILES) : $(INT_DIR)/release/cpp_util_lib_test/%.o : $(SRC_DIR)/cpp_util_lib_test/%.cpp $(INT_DIR)/release/cpp_util_lib_test/%.d | $(INT_DIR)/release/cpp_util_lib_test
	$(CXX) $(CXXFLAGS) $(release_FLAGS) -c -o $@ $<

$(BIN_DIR)/release/test/cpp_util_lib_test : $(release_cpp_util_lib_test_OBJ_FILES) $(INT_DIR)/release/test_lib.a $(INT_DIR)/release/test_main_lib.a | $(BIN_DIR)/release/test
	$(CXX) $(LDFLAGS) $(release_FLAGS) -o $@ $^

$(INT_DIR)/release/cpp_util_lib_test/success.run : $(BIN_DIR)/release/test/cpp_util_lib_test | $(INT_DIR)/release/cpp_util_lib_test
	$^
	touch $@

release : $(INT_DIR)/release/cpp_util_lib_test/success.run

DEP_FILES += $(release_cpp_util_lib_test_OBJ_FILES:.o=.d)

# Rules for cstdio_lib

cstdio_lib_CPP_FILES := $(wildcard $(SRC_DIR)/cstdio_lib/*.cpp)

debug_cstdio_lib_OBJ_FILES := $(cstdio_lib_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/debug/%.o)

$(debug_cstdio_lib_OBJ_FILES) : $(INT_DIR)/debug/cstdio_lib/%.o : $(SRC_DIR)/cstdio_lib/%.cpp $(INT_DIR)/debug/cstdio_lib/%.d | $(INT_DIR)/debug/cstdio_lib
	$(CXX) $(CXXFLAGS) $(debug_FLAGS) -c -o $@ $<

$(INT_DIR)/debug/cstdio_lib.a : $(debug_cstdio_lib_OBJ_FILES) | $(INT_DIR)/debug
	ar rcs $@ $^

debug : $(INT_DIR)/debug/cstdio_lib.a

DEP_FILES += $(debug_cstdio_lib_OBJ_FILES:.o=.d)

release_cstdio_lib_OBJ_FILES := $(cstdio_lib_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/release/%.o)

$(release_cstdio_lib_OBJ_FILES) : $(INT_DIR)/release/cstdio_lib/%.o : $(SRC_DIR)/cstdio_lib/%.cpp $(INT_DIR)/release/cstdio_lib/%.d | $(INT_DIR)/release/cstdio_lib
	$(CXX) $(CXXFLAGS) $(release_FLAGS) -c -o $@ $<

$(INT_DIR)/release/cstdio_lib.a : $(release_cstdio_lib_OBJ_FILES) | $(INT_DIR)/release
	ar rcs $@ $^

release : $(INT_DIR)/release/cstdio_lib.a

DEP_FILES += $(release_cstdio_lib_OBJ_FILES:.o=.d)

# Rules for cstdio_lib_test

cstdio_lib_test_CPP_FILES := $(wildcard $(SRC_DIR)/cstdio_lib_test/*.cpp)

debug_cstdio_lib_test_OBJ_FILES := $(cstdio_lib_test_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/debug/%.o)

$(debug_cstdio_lib_test_OBJ_FILES) : $(INT_DIR)/debug/cstdio_lib_test/%.o : $(SRC_DIR)/cstdio_lib_test/%.cpp $(INT_DIR)/debug/cstdio_lib_test/%.d | $(INT_DIR)/debug/cstdio_lib_test
	$(CXX) $(CXXFLAGS) $(debug_FLAGS) -c -o $@ $<

$(BIN_DIR)/debug/test/cstdio_lib_test : $(debug_cstdio_lib_test_OBJ_FILES) $(INT_DIR)/debug/cstdio_lib.a $(INT_DIR)/debug/test_lib.a $(INT_DIR)/debug/test_main_lib.a | $(BIN_DIR)/debug/test
	$(CXX) $(LDFLAGS) $(debug_FLAGS) -o $@ $^

$(INT_DIR)/debug/cstdio_lib_test/success.run : $(BIN_DIR)/debug/test/cstdio_lib_test | $(INT_DIR)/debug/cstdio_lib_test
	$^
	touch $@

debug : $(INT_DIR)/debug/cstdio_lib_test/success.run

DEP_FILES += $(debug_cstdio_lib_test_OBJ_FILES:.o=.d)

release_cstdio_lib_test_OBJ_FILES := $(cstdio_lib_test_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/release/%.o)

$(release_cstdio_lib_test_OBJ_FILES) : $(INT_DIR)/release/cstdio_lib_test/%.o : $(SRC_DIR)/cstdio_lib_test/%.cpp $(INT_DIR)/release/cstdio_lib_test/%.d | $(INT_DIR)/release/cstdio_lib_test
	$(CXX) $(CXXFLAGS) $(release_FLAGS) -c -o $@ $<

$(BIN_DIR)/release/test/cstdio_lib_test : $(release_cstdio_lib_test_OBJ_FILES) $(INT_DIR)/release/cstdio_lib.a $(INT_DIR)/release/test_lib.a $(INT_DIR)/release/test_main_lib.a | $(BIN_DIR)/release/test
	$(CXX) $(LDFLAGS) $(release_FLAGS) -o $@ $^

$(INT_DIR)/release/cstdio_lib_test/success.run : $(BIN_DIR)/release/test/cstdio_lib_test | $(INT_DIR)/release/cstdio_lib_test
	$^
	touch $@

release : $(INT_DIR)/release/cstdio_lib_test/success.run

DEP_FILES += $(release_cstdio_lib_test_OBJ_FILES:.o=.d)

# Rules for fibonacci

fibonacci_CPP_FILES := $(wildcard $(SRC_DIR)/fibonacci/*.cpp)

debug_fibonacci_OBJ_FILES := $(fibonacci_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/debug/%.o)

$(debug_fibonacci_OBJ_FILES) : $(INT_DIR)/debug/fibonacci/%.o : $(SRC_DIR)/fibonacci/%.cpp $(INT_DIR)/debug/fibonacci/%.d | $(INT_DIR)/debug/fibonacci
	$(CXX) $(CXXFLAGS) $(debug_FLAGS) -c -o $@ $<

$(BIN_DIR)/debug/fibonacci : $(debug_fibonacci_OBJ_FILES) $(INT_DIR)/debug/fibonacci_lib.a | $(BIN_DIR)/debug
	$(CXX) $(LDFLAGS) $(debug_FLAGS) -o $@ $^

debug : $(BIN_DIR)/debug/fibonacci

DEP_FILES += $(debug_fibonacci_OBJ_FILES:.o=.d)

release_fibonacci_OBJ_FILES := $(fibonacci_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/release/%.o)

$(release_fibonacci_OBJ_FILES) : $(INT_DIR)/release/fibonacci/%.o : $(SRC_DIR)/fibonacci/%.cpp $(INT_DIR)/release/fibonacci/%.d | $(INT_DIR)/release/fibonacci
	$(CXX) $(CXXFLAGS) $(release_FLAGS) -c -o $@ $<

$(BIN_DIR)/release/fibonacci : $(release_fibonacci_OBJ_FILES) $(INT_DIR)/release/fibonacci_lib.a | $(BIN_DIR)/release
	$(CXX) $(LDFLAGS) $(release_FLAGS) -o $@ $^

release : $(BIN_DIR)/release/fibonacci

DEP_FILES += $(release_fibonacci_OBJ_FILES:.o=.d)

# Rules for fibonacci_lib

fibonacci_lib_CPP_FILES := $(wildcard $(SRC_DIR)/fibonacci_lib/*.cpp)

debug_fibonacci_lib_OBJ_FILES := $(fibonacci_lib_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/debug/%.o)

$(debug_fibonacci_lib_OBJ_FILES) : $(INT_DIR)/debug/fibonacci_lib/%.o : $(SRC_DIR)/fibonacci_lib/%.cpp $(INT_DIR)/debug/fibonacci_lib/%.d | $(INT_DIR)/debug/fibonacci_lib
	$(CXX) $(CXXFLAGS) $(debug_FLAGS) -c -o $@ $<

$(INT_DIR)/debug/fibonacci_lib.a : $(debug_fibonacci_lib_OBJ_FILES) | $(INT_DIR)/debug
	ar rcs $@ $^

debug : $(INT_DIR)/debug/fibonacci_lib.a

DEP_FILES += $(debug_fibonacci_lib_OBJ_FILES:.o=.d)

release_fibonacci_lib_OBJ_FILES := $(fibonacci_lib_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/release/%.o)

$(release_fibonacci_lib_OBJ_FILES) : $(INT_DIR)/release/fibonacci_lib/%.o : $(SRC_DIR)/fibonacci_lib/%.cpp $(INT_DIR)/release/fibonacci_lib/%.d | $(INT_DIR)/release/fibonacci_lib
	$(CXX) $(CXXFLAGS) $(release_FLAGS) -c -o $@ $<

$(INT_DIR)/release/fibonacci_lib.a : $(release_fibonacci_lib_OBJ_FILES) | $(INT_DIR)/release
	ar rcs $@ $^

release : $(INT_DIR)/release/fibonacci_lib.a

DEP_FILES += $(release_fibonacci_lib_OBJ_FILES:.o=.d)

# Rules for fibonacci_lib_test

fibonacci_lib_test_CPP_FILES := $(wildcard $(SRC_DIR)/fibonacci_lib_test/*.cpp)

debug_fibonacci_lib_test_OBJ_FILES := $(fibonacci_lib_test_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/debug/%.o)

$(debug_fibonacci_lib_test_OBJ_FILES) : $(INT_DIR)/debug/fibonacci_lib_test/%.o : $(SRC_DIR)/fibonacci_lib_test/%.cpp $(INT_DIR)/debug/fibonacci_lib_test/%.d | $(INT_DIR)/debug/fibonacci_lib_test
	$(CXX) $(CXXFLAGS) $(debug_FLAGS) -c -o $@ $<

$(BIN_DIR)/debug/test/fibonacci_lib_test : $(debug_fibonacci_lib_test_OBJ_FILES) $(INT_DIR)/debug/fibonacci_lib.a $(INT_DIR)/debug/test_lib.a $(INT_DIR)/debug/test_main_lib.a | $(BIN_DIR)/debug/test
	$(CXX) $(LDFLAGS) $(debug_FLAGS) -o $@ $^

$(INT_DIR)/debug/fibonacci_lib_test/success.run : $(BIN_DIR)/debug/test/fibonacci_lib_test | $(INT_DIR)/debug/fibonacci_lib_test
	$^
	touch $@

debug : $(INT_DIR)/debug/fibonacci_lib_test/success.run

DEP_FILES += $(debug_fibonacci_lib_test_OBJ_FILES:.o=.d)

release_fibonacci_lib_test_OBJ_FILES := $(fibonacci_lib_test_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/release/%.o)

$(release_fibonacci_lib_test_OBJ_FILES) : $(INT_DIR)/release/fibonacci_lib_test/%.o : $(SRC_DIR)/fibonacci_lib_test/%.cpp $(INT_DIR)/release/fibonacci_lib_test/%.d | $(INT_DIR)/release/fibonacci_lib_test
	$(CXX) $(CXXFLAGS) $(release_FLAGS) -c -o $@ $<

$(BIN_DIR)/release/test/fibonacci_lib_test : $(release_fibonacci_lib_test_OBJ_FILES) $(INT_DIR)/release/fibonacci_lib.a $(INT_DIR)/release/test_lib.a $(INT_DIR)/release/test_main_lib.a | $(BIN_DIR)/release/test
	$(CXX) $(LDFLAGS) $(release_FLAGS) -o $@ $^

$(INT_DIR)/release/fibonacci_lib_test/success.run : $(BIN_DIR)/release/test/fibonacci_lib_test | $(INT_DIR)/release/fibonacci_lib_test
	$^
	touch $@

release : $(INT_DIR)/release/fibonacci_lib_test/success.run

DEP_FILES += $(release_fibonacci_lib_test_OBJ_FILES:.o=.d)

# Rules for how_vector_works

how_vector_works_CPP_FILES := $(wildcard $(SRC_DIR)/how_vector_works/*.cpp)

debug_how_vector_works_OBJ_FILES := $(how_vector_works_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/debug/%.o)

$(debug_how_vector_works_OBJ_FILES) : $(INT_DIR)/debug/how_vector_works/%.o : $(SRC_DIR)/how_vector_works/%.cpp $(INT_DIR)/debug/how_vector_works/%.d | $(INT_DIR)/debug/how_vector_works
	$(CXX) $(CXXFLAGS) $(debug_FLAGS) -c -o $@ $<

$(BIN_DIR)/debug/how_vector_works : $(debug_how_vector_works_OBJ_FILES)  | $(BIN_DIR)/debug
	$(CXX) $(LDFLAGS) $(debug_FLAGS) -o $@ $^

debug : $(BIN_DIR)/debug/how_vector_works

DEP_FILES += $(debug_how_vector_works_OBJ_FILES:.o=.d)

release_how_vector_works_OBJ_FILES := $(how_vector_works_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/release/%.o)

$(release_how_vector_works_OBJ_FILES) : $(INT_DIR)/release/how_vector_works/%.o : $(SRC_DIR)/how_vector_works/%.cpp $(INT_DIR)/release/how_vector_works/%.d | $(INT_DIR)/release/how_vector_works
	$(CXX) $(CXXFLAGS) $(release_FLAGS) -c -o $@ $<

$(BIN_DIR)/release/how_vector_works : $(release_how_vector_works_OBJ_FILES)  | $(BIN_DIR)/release
	$(CXX) $(LDFLAGS) $(release_FLAGS) -o $@ $^

release : $(BIN_DIR)/release/how_vector_works

DEP_FILES += $(release_how_vector_works_OBJ_FILES:.o=.d)

# Rules for sedgewick_lib_test

sedgewick_lib_test_CPP_FILES := $(wildcard $(SRC_DIR)/sedgewick_lib_test/*.cpp)

debug_sedgewick_lib_test_OBJ_FILES := $(sedgewick_lib_test_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/debug/%.o)

$(debug_sedgewick_lib_test_OBJ_FILES) : $(INT_DIR)/debug/sedgewick_lib_test/%.o : $(SRC_DIR)/sedgewick_lib_test/%.cpp $(INT_DIR)/debug/sedgewick_lib_test/%.d | $(INT_DIR)/debug/sedgewick_lib_test
	$(CXX) $(CXXFLAGS) $(debug_FLAGS) -c -o $@ $<

$(BIN_DIR)/debug/test/sedgewick_lib_test : $(debug_sedgewick_lib_test_OBJ_FILES) $(INT_DIR)/debug/test_lib.a $(INT_DIR)/debug/test_main_lib.a | $(BIN_DIR)/debug/test
	$(CXX) $(LDFLAGS) $(debug_FLAGS) -o $@ $^

$(INT_DIR)/debug/sedgewick_lib_test/success.run : $(BIN_DIR)/debug/test/sedgewick_lib_test | $(INT_DIR)/debug/sedgewick_lib_test
	$^
	touch $@

debug : $(INT_DIR)/debug/sedgewick_lib_test/success.run

DEP_FILES += $(debug_sedgewick_lib_test_OBJ_FILES:.o=.d)

release_sedgewick_lib_test_OBJ_FILES := $(sedgewick_lib_test_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/release/%.o)

$(release_sedgewick_lib_test_OBJ_FILES) : $(INT_DIR)/release/sedgewick_lib_test/%.o : $(SRC_DIR)/sedgewick_lib_test/%.cpp $(INT_DIR)/release/sedgewick_lib_test/%.d | $(INT_DIR)/release/sedgewick_lib_test
	$(CXX) $(CXXFLAGS) $(release_FLAGS) -c -o $@ $<

$(BIN_DIR)/release/test/sedgewick_lib_test : $(release_sedgewick_lib_test_OBJ_FILES) $(INT_DIR)/release/test_lib.a $(INT_DIR)/release/test_main_lib.a | $(BIN_DIR)/release/test
	$(CXX) $(LDFLAGS) $(release_FLAGS) -o $@ $^

$(INT_DIR)/release/sedgewick_lib_test/success.run : $(BIN_DIR)/release/test/sedgewick_lib_test | $(INT_DIR)/release/sedgewick_lib_test
	$^
	touch $@

release : $(INT_DIR)/release/sedgewick_lib_test/success.run

DEP_FILES += $(release_sedgewick_lib_test_OBJ_FILES:.o=.d)

# Rules for test_lib

test_lib_CPP_FILES := $(wildcard $(SRC_DIR)/test_lib/*.cpp)

debug_test_lib_OBJ_FILES := $(test_lib_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/debug/%.o)

$(debug_test_lib_OBJ_FILES) : $(INT_DIR)/debug/test_lib/%.o : $(SRC_DIR)/test_lib/%.cpp $(INT_DIR)/debug/test_lib/%.d | $(INT_DIR)/debug/test_lib
	$(CXX) $(CXXFLAGS) $(debug_FLAGS) -c -o $@ $<

$(INT_DIR)/debug/test_lib.a : $(debug_test_lib_OBJ_FILES) | $(INT_DIR)/debug
	ar rcs $@ $^

debug : $(INT_DIR)/debug/test_lib.a

DEP_FILES += $(debug_test_lib_OBJ_FILES:.o=.d)

release_test_lib_OBJ_FILES := $(test_lib_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/release/%.o)

$(release_test_lib_OBJ_FILES) : $(INT_DIR)/release/test_lib/%.o : $(SRC_DIR)/test_lib/%.cpp $(INT_DIR)/release/test_lib/%.d | $(INT_DIR)/release/test_lib
	$(CXX) $(CXXFLAGS) $(release_FLAGS) -c -o $@ $<

$(INT_DIR)/release/test_lib.a : $(release_test_lib_OBJ_FILES) | $(INT_DIR)/release
	ar rcs $@ $^

release : $(INT_DIR)/release/test_lib.a

DEP_FILES += $(release_test_lib_OBJ_FILES:.o=.d)

# Rules for test_main_lib

test_main_lib_CPP_FILES := $(wildcard $(SRC_DIR)/test_main_lib/*.cpp)

debug_test_main_lib_OBJ_FILES := $(test_main_lib_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/debug/%.o)

$(debug_test_main_lib_OBJ_FILES) : $(INT_DIR)/debug/test_main_lib/%.o : $(SRC_DIR)/test_main_lib/%.cpp $(INT_DIR)/debug/test_main_lib/%.d | $(INT_DIR)/debug/test_main_lib
	$(CXX) $(CXXFLAGS) $(debug_FLAGS) -c -o $@ $<

$(INT_DIR)/debug/test_main_lib.a : $(debug_test_main_lib_OBJ_FILES) | $(INT_DIR)/debug
	ar rcs $@ $^

debug : $(INT_DIR)/debug/test_main_lib.a

DEP_FILES += $(debug_test_main_lib_OBJ_FILES:.o=.d)

release_test_main_lib_OBJ_FILES := $(test_main_lib_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/release/%.o)

$(release_test_main_lib_OBJ_FILES) : $(INT_DIR)/release/test_main_lib/%.o : $(SRC_DIR)/test_main_lib/%.cpp $(INT_DIR)/release/test_main_lib/%.d | $(INT_DIR)/release/test_main_lib
	$(CXX) $(CXXFLAGS) $(release_FLAGS) -c -o $@ $<

$(INT_DIR)/release/test_main_lib.a : $(release_test_main_lib_OBJ_FILES) | $(INT_DIR)/release
	ar rcs $@ $^

release : $(INT_DIR)/release/test_main_lib.a

DEP_FILES += $(release_test_main_lib_OBJ_FILES:.o=.d)

# Folders creation
$(BIN_DIR) $(INT_DIR):
	mkdir $@

$(BIN_DIR)/debug : | $(BIN_DIR)
	mkdir $@

$(BIN_DIR)/debug/test : | $(BIN_DIR)/debug
	mkdir $@

$(INT_DIR)/debug : | $(INT_DIR)
	mkdir $@

$(INT_DIR)/debug/clrs_lib_test : | $(INT_DIR)/debug
	mkdir $@

$(INT_DIR)/debug/coro_lib_perf : | $(INT_DIR)/debug
	mkdir $@

$(INT_DIR)/debug/coro_lib_test : | $(INT_DIR)/debug
	mkdir $@

$(INT_DIR)/debug/cpp_util_lib_test : | $(INT_DIR)/debug
	mkdir $@

$(INT_DIR)/debug/cstdio_lib : | $(INT_DIR)/debug
	mkdir $@

$(INT_DIR)/debug/cstdio_lib_test : | $(INT_DIR)/debug
	mkdir $@

$(INT_DIR)/debug/fibonacci : | $(INT_DIR)/debug
	mkdir $@

$(INT_DIR)/debug/fibonacci_lib : | $(INT_DIR)/debug
	mkdir $@

$(INT_DIR)/debug/fibonacci_lib_test : | $(INT_DIR)/debug
	mkdir $@

$(INT_DIR)/debug/how_vector_works : | $(INT_DIR)/debug
	mkdir $@

$(INT_DIR)/debug/sedgewick_lib_test : | $(INT_DIR)/debug
	mkdir $@

$(INT_DIR)/debug/test_lib : | $(INT_DIR)/debug
	mkdir $@

$(INT_DIR)/debug/test_main_lib : | $(INT_DIR)/debug
	mkdir $@

$(BIN_DIR)/release : | $(BIN_DIR)
	mkdir $@

$(BIN_DIR)/release/test : | $(BIN_DIR)/release
	mkdir $@

$(INT_DIR)/release : | $(INT_DIR)
	mkdir $@

$(INT_DIR)/release/clrs_lib_test : | $(INT_DIR)/release
	mkdir $@

$(INT_DIR)/release/coro_lib_perf : | $(INT_DIR)/release
	mkdir $@

$(INT_DIR)/release/coro_lib_test : | $(INT_DIR)/release
	mkdir $@

$(INT_DIR)/release/cpp_util_lib_test : | $(INT_DIR)/release
	mkdir $@

$(INT_DIR)/release/cstdio_lib : | $(INT_DIR)/release
	mkdir $@

$(INT_DIR)/release/cstdio_lib_test : | $(INT_DIR)/release
	mkdir $@

$(INT_DIR)/release/fibonacci : | $(INT_DIR)/release
	mkdir $@

$(INT_DIR)/release/fibonacci_lib : | $(INT_DIR)/release
	mkdir $@

$(INT_DIR)/release/fibonacci_lib_test : | $(INT_DIR)/release
	mkdir $@

$(INT_DIR)/release/how_vector_works : | $(INT_DIR)/release
	mkdir $@

$(INT_DIR)/release/sedgewick_lib_test : | $(INT_DIR)/release
	mkdir $@

$(INT_DIR)/release/test_lib : | $(INT_DIR)/release
	mkdir $@

$(INT_DIR)/release/test_main_lib : | $(INT_DIR)/release
	mkdir $@

## To clean and build run 'make clean && make'
clean:
	rm -rf $(BIN_DIR) $(INT_DIR) $(TMP_DIR)

# Dependency files
## Do not fail is missing (e.g. initially or deleted): we require it by the compile rule
$(DEP_FILES): $(INT_DIR)/%.d: ;

## Include dependency files (ignore them if missing)
-include $(DEP_FILES)
