#!/usr/bin/env python3

import os

def write_makefile(out):
    configs = ["debug", "release"]

    projects = [
        ("fibonacci", ["fibonacci_lib"]),
        ("fibonacci_lib", []),
        ("fibonacci_lib_test", ["fibonacci_lib", "test_lib", "test_main_lib"]),
        ("test_lib", []),
        ("test_main_lib", []),
    ]

    out.write('''\
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
## -MP creates phony targtes for headers (deals with deleted headers after
##  obj file has been compiled)
## -MT specifies the dependency target (path qualified obj file name)
DEP_FLAGS = -MT $@ -MMD -MP -MF $(@:.o=.d)
STD_FLAGS = --std=c++17 -pthread -fno-rtti
WARN_FLAGS = -Wall -Werror
debug_FLAGS = -g
release_FLAGS = -O3
CXXFLAGS = $(STD_FLAGS) $(DEP_FLAGS) $(WARN_FLAGS)
LDFLAGS = $(STD_FLAGS) $(WARN_FLAGS)

## Compilation rule for .o files has a dependency on .d file to ensure that if
## the .d file is deleted, the object file is created again e.g. in case a header is changed

# Rules on how to build
## To build all 'make'
.DEFAULT: all

.PHONY: all {configs} clean

all : {configs}

{configs} :

DEP_FILES :=
\n'''.format(configs=" ".join(configs)))

    for project, libs in projects:
        out.write('''\
# Rules for {project}

{project}_CPP_FILES := $(wildcard $(SRC_DIR)/{project}/*.cpp)
\n'''.format(project=project))
        
        for config in configs:
                out.write('''\
{config}_{project}_OBJ_FILES := $({project}_CPP_FILES:$(SRC_DIR)/%.cpp=$(INT_DIR)/{config}/%.o)

$({config}_{project}_OBJ_FILES) : $(INT_DIR)/{config}/{project}/%.o : $(SRC_DIR)/{project}/%.cpp $(INT_DIR)/{config}/{project}/%.d | $(INT_DIR)/{config}/{project}
\t$(CXX) $(CXXFLAGS) $({config}_FLAGS) -c -o $@ $<
\n'''.format(project=project, config=config))

                if project.endswith("_lib"):
                    out.write('''\
$(INT_DIR)/{config}/{project}.a : $({config}_{project}_OBJ_FILES) | $(INT_DIR)/{config}
\tar rcs $@ $^

{config} : $(INT_DIR)/{config}/{project}.a
\n'''.format(project=project, config=config))
                elif project.endswith("_lib_test"):
                    config_libs = " ".join("$(INT_DIR)/" + config + "/" + lib + ".a" for lib in libs)

                    out.write('''\
$(BIN_DIR)/{config}/test/{project} : $({config}_{project}_OBJ_FILES) {config_libs} | $(BIN_DIR)/{config}/test
\t$(CXX) $(LDFLAGS) $({config}_FLAGS) -o $@ $^

$(INT_DIR)/{config}/{project}/success.run : $(BIN_DIR)/{config}/test/{project} | $(INT_DIR)/{config}/{project}
\t$^
\ttouch $@

{config} : $(INT_DIR)/{config}/{project}/success.run
\n'''.format(project=project, config=config, config_libs=config_libs))
                else:
                    config_libs = " ".join("$(INT_DIR)/" + config + "/" + lib + ".a" for lib in libs)

                    out.write('''\
$(BIN_DIR)/{config}/{project} : $({config}_{project}_OBJ_FILES) {config_libs} | $(BIN_DIR)/{config}
\t$(CXX) $(LDFLAGS) $({config}_FLAGS) -o $@ $^

{config} : $(BIN_DIR)/{config}/{project}
\n'''.format(project=project, config=config, config_libs=config_libs))

                out.write('''\
DEP_FILES += $({config}_{project}_OBJ_FILES:.o=.d)
\n'''.format(project=project, config=config))

    out.write('''\
# Folders creation
$(BIN_DIR) $(INT_DIR):
\tmkdir $@
\n''')
    for config in configs:
        out.write('''\
$(BIN_DIR)/{config} : | $(BIN_DIR)
\tmkdir $@

$(BIN_DIR)/{config}/test : | $(BIN_DIR)/{config}
\tmkdir $@

$(INT_DIR)/{config} : | $(INT_DIR)
\tmkdir $@
\n'''.format(config=config))

        for project, _ in projects:
            out.write('''\
$(INT_DIR)/{config}/{project} : | $(INT_DIR)/{config}
\tmkdir $@
\n'''.format(project=project,config=config))
        
    out.write('''\
## To clean and build run 'make clean && make'
clean:
\trm -rf $(BIN_DIR) $(INT_DIR) $(TMP_DIR)

# Dependency files
## Do not fail is missing (e.g. initially or deleted): we require it by the compile rule
$(DEP_FILES): $(INT_DIR)/%.d: ;

## Include dependency files (ignore them if missing)
-include $(DEP_FILES)
'''.format(configs=", ".join(configs)))


if __name__ == "__main__":
    make_file_name = os.path.join(os.path.dirname(__file__), "..", "makefile")
    with open(make_file_name, 'w') as makefile:
        write_makefile(makefile)