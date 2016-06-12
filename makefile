# Delete the default sufixes (otherwise visible in 'make -d')
.SUFFIXES:

# Cancel source control implicit rules (otherwise visible in 'make -d')
%: %,v
%: RCS/%,v
%: RCS/%
%: s.%
%: SCCS/s.%

# Folders used
## contains executables we build
BIN_DIR = bin
## source files expected here
SRC_DIR = src
## temp folder
TMP_DIR = tmp
## build folder e.g. for object files and dependency files
BUILD_DIR = $(TMP_DIR)/build
## folder for tests
TEST_DIR = $(TMP_DIR)/test

# Executable name
TARGET = prog

# Compiler flags
CXX = g++
## -MMD creates dependency list, but ignores system includes
## -MF specifies where to create the dependency file name
## -MP creates phony targes for headers (deals with deleted headers after
##  obj file has been compiled)
## -MT specifies the dependency target (path qualified obj file name)
DEP_FLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
STD_FLAGS = -std=c++1y -pthread -O3
WARN_FLAGS = -Wall -Wextra -pedantic -Werror -Wsign-conversion -Wconversion -Wshadow
CXXFLAGS = $(STD_FLAGS) $(DEP_FLAGS) $(WARN_FLAGS)
LDFLAGS = $(STD_FLAGS) $(WARN_FLAGS)

# Things to build
BIN_TARGET = $(BIN_DIR)/$(TARGET)
CPP_FILES := $(shell find $(SRC_DIR) -name '*.cpp')
OBJ_FILES := $(CPP_FILES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
DEP_FILES := $(CPP_FILES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.d)

# Rules on how to build

## To build all 'make all'
.DEFAULT: run

.PHONY: all, clean, run

## To build and run the program 'make run'
run: all | $(TEST_DIR)
	$(BIN_TARGET)

all: $(BIN_TARGET)

## Compilation rule (dependency on .d file ensures that if the .d file
## is deleted, the obj file is created again in case a header is changed)
$(OBJ_FILES): $(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(BUILD_DIR)/%.d | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

## Linkage rule
$(BIN_TARGET): $(OBJ_FILES) | $(BIN_DIR)
	$(CXX) $(LDFLAGS) -o $@ $^

## Folders creation
$(BUILD_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(TEST_DIR):
	mkdir -p $@

## To clean and build run 'make clean && make'
clean:
	rm -rf $(BIN_DIR) $(TMP_DIR)


## Do not fail when dependency file is deleted (it is required by the compile
## rule)
$(DEP_FILES): $(BUILD_DIR)/%.d: ;

# Include dependency files (ignore them if missing)
-include $(DEP_FILES)
