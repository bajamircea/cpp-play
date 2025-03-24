# CPP play

This is a play area on an variety of C++ topics

## Unique handle

[The ultimate RAII helper for dealing with C handles](src/cpp_util_lib/unique_handle.md)

## C++20 coroutine prodding

[See src/coro_st_lib, src/coro_st_lib_test](src/coro_st_lib/README.md)

## How vector works

[Prodding the std::vector](src/how_vector_works/README.md)

## Fibonacci

What would it mean to do Sean Parent's sub 1 second Fibonacci calculation
for 1 million [from scratch?](src/fibonacci/README.md)

## Project structure

- `build` - see `generate_makefile.py` which generates/overrides the makefile
- `src` - the source folder, underneath there folders which are are:
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
