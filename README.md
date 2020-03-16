# CPP play

This is a play area on an variety of C++ topics

## Project structure

- `build` - see `generate_makefile.py` which generates/overrides the makefile
- `src` - the source folder underneath there folders which are are:
  - `xyz` - plain executable projects
  - `xyz_lib` - static libraries
  - `xyz_test` - unit tests
  - `test_lib` - a library for unit tests
  - `test_main_lib` - a library containing the `main` function for unit tests
  - `old` - various older attempts
- `makefile` - run to build
- `bin` - binaries are build here
  - under `debug` and `release`
- `int` - intermediate build folder for object files
- `.vscode` - settings for building, debuging and indentation

## Fibonacci

[See more details](src/fibonacci/README.md)