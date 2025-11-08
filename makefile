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
## It seems that g++ on -O3 has some false positives on dangling pointer errors
release_FLAGS = -O3 -march=native -g
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

# Rules for coro_st_lib_test

coro_st_lib_test_CPP_FILES := $(wildcard $(SRC_DIR)/coro_st_lib_test/*.cpp)

debug_coro_st_lib_test_OBJ_FILES := $(coro_st_lib_test_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/debug/%.o)

$(debug_coro_st_lib_test_OBJ_FILES) : $(INT_DIR)/debug/coro_st_lib_test/%.o : $(SRC_DIR)/coro_st_lib_test/%.cpp $(INT_DIR)/debug/coro_st_lib_test/%.d | $(INT_DIR)/debug/coro_st_lib_test
	$(CXX) $(CXXFLAGS) $(debug_FLAGS) -c -o $@ $<

$(BIN_DIR)/debug/test/coro_st_lib_test : $(debug_coro_st_lib_test_OBJ_FILES)  | $(BIN_DIR)/debug/test
	$(CXX) $(LDFLAGS) $(debug_FLAGS) -o $@ $^

$(INT_DIR)/debug/coro_st_lib_test/success.run : $(BIN_DIR)/debug/test/coro_st_lib_test | $(INT_DIR)/debug/coro_st_lib_test
	$^
	touch $@

debug : $(INT_DIR)/debug/coro_st_lib_test/success.run

DEP_FILES += $(debug_coro_st_lib_test_OBJ_FILES:.o=.d)

release_coro_st_lib_test_OBJ_FILES := $(coro_st_lib_test_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/release/%.o)

$(release_coro_st_lib_test_OBJ_FILES) : $(INT_DIR)/release/coro_st_lib_test/%.o : $(SRC_DIR)/coro_st_lib_test/%.cpp $(INT_DIR)/release/coro_st_lib_test/%.d | $(INT_DIR)/release/coro_st_lib_test
	$(CXX) $(CXXFLAGS) $(release_FLAGS) -c -o $@ $<

$(BIN_DIR)/release/test/coro_st_lib_test : $(release_coro_st_lib_test_OBJ_FILES)  | $(BIN_DIR)/release/test
	$(CXX) $(LDFLAGS) $(release_FLAGS) -o $@ $^

$(INT_DIR)/release/coro_st_lib_test/success.run : $(BIN_DIR)/release/test/coro_st_lib_test | $(INT_DIR)/release/coro_st_lib_test
	$^
	touch $@

release : $(INT_DIR)/release/coro_st_lib_test/success.run

DEP_FILES += $(release_coro_st_lib_test_OBJ_FILES:.o=.d)

# Folders creation
$(BIN_DIR) $(INT_DIR):
	mkdir $@

$(BIN_DIR)/debug : | $(BIN_DIR)
	mkdir $@

$(BIN_DIR)/debug/test : | $(BIN_DIR)/debug
	mkdir $@

$(INT_DIR)/debug : | $(INT_DIR)
	mkdir $@

$(INT_DIR)/debug/coro_st_lib_test : | $(INT_DIR)/debug
	mkdir $@

$(BIN_DIR)/release : | $(BIN_DIR)
	mkdir $@

$(BIN_DIR)/release/test : | $(BIN_DIR)/release
	mkdir $@

$(INT_DIR)/release : | $(INT_DIR)
	mkdir $@

$(INT_DIR)/release/coro_st_lib_test : | $(INT_DIR)/release
	mkdir $@

## To clean and build run 'make clean && make'
clean:
	rm -rf $(BIN_DIR) $(INT_DIR) $(TMP_DIR)

# Dependency files
## Do not fail is missing (e.g. initially or deleted): we require it by the compile rule
$(DEP_FILES): $(INT_DIR)/%.d: ;

## Include dependency files (ignore them if missing)
-include $(DEP_FILES)
