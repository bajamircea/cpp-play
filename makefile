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

# Rules for repro_test

repro_test_CPP_FILES := $(wildcard $(SRC_DIR)/repro_test/*.cpp)

debug_repro_test_OBJ_FILES := $(repro_test_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/debug/%.o)

$(debug_repro_test_OBJ_FILES) : $(INT_DIR)/debug/repro_test/%.o : $(SRC_DIR)/repro_test/%.cpp $(INT_DIR)/debug/repro_test/%.d | $(INT_DIR)/debug/repro_test
	$(CXX) $(CXXFLAGS) $(debug_FLAGS) -c -o $@ $<

$(BIN_DIR)/debug/repro_test : $(debug_repro_test_OBJ_FILES)  | $(BIN_DIR)/debug
	$(CXX) $(LDFLAGS) $(debug_FLAGS) -o $@ $^

debug : $(BIN_DIR)/debug/repro_test

DEP_FILES += $(debug_repro_test_OBJ_FILES:.o=.d)

release_repro_test_OBJ_FILES := $(repro_test_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/release/%.o)

$(release_repro_test_OBJ_FILES) : $(INT_DIR)/release/repro_test/%.o : $(SRC_DIR)/repro_test/%.cpp $(INT_DIR)/release/repro_test/%.d | $(INT_DIR)/release/repro_test
	$(CXX) $(CXXFLAGS) $(release_FLAGS) -c -o $@ $<

$(BIN_DIR)/release/repro_test : $(release_repro_test_OBJ_FILES)  | $(BIN_DIR)/release
	$(CXX) $(LDFLAGS) $(release_FLAGS) -o $@ $^

release : $(BIN_DIR)/release/repro_test

DEP_FILES += $(release_repro_test_OBJ_FILES:.o=.d)

# Folders creation
$(BIN_DIR) $(INT_DIR):
	mkdir $@

$(BIN_DIR)/debug : | $(BIN_DIR)
	mkdir $@

$(BIN_DIR)/debug/test : | $(BIN_DIR)/debug
	mkdir $@

$(INT_DIR)/debug : | $(INT_DIR)
	mkdir $@

$(INT_DIR)/debug/repro_test : | $(INT_DIR)/debug
	mkdir $@

$(BIN_DIR)/release : | $(BIN_DIR)
	mkdir $@

$(BIN_DIR)/release/test : | $(BIN_DIR)/release
	mkdir $@

$(INT_DIR)/release : | $(INT_DIR)
	mkdir $@

$(INT_DIR)/release/repro_test : | $(INT_DIR)/release
	mkdir $@

## To clean and build run 'make clean && make'
clean:
	rm -rf $(BIN_DIR) $(INT_DIR) $(TMP_DIR)

# Dependency files
## Do not fail is missing (e.g. initially or deleted): we require it by the compile rule
$(DEP_FILES): $(INT_DIR)/%.d: ;

## Include dependency files (ignore them if missing)
-include $(DEP_FILES)
