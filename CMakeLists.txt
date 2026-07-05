cmake_minimum_required(VERSION 3.15...4.1)
project(reactor_cascade LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0)
FetchContent_MakeAvailable(googletest)

enable_testing()


# calclib: target's name (handle for within this file to refer to) STATIC: makes
# .a file (if SHARED then .so, if INTERFACE it produces nothing, as this
# indicates we have a header only library) src/calclib.cpp: the source file
# being compiled into the library include/calc/lib.hpp: only for IDE
# referencing, no code functionality NOTE: The file will appear as libcalclib.a
# inside /build, as prefix "lib" is added by CMake, as well as .a on linux
# systems

# add_library(calclib STATIC src/calclib.cpp include/calc/lib.hpp)

# include contains the .hpp headers needed to satisfy the compiler at
# compilation time. When calclibs source files (src/calclib.cpp) get compiled
# the headers are made available via -I include/ (this would also be the case
# with PRIVATE) Due to keyword PUBLIC, the headers in calclib are also made
# available with -I include/ to any *target* within this call of *cmake* that
# links against libcalclib.a

# target_include_directories(calclib PUBLIC include)

# indicates that the source files that are part of *target* library calclib,
# later libcalclib.a need to be compiled according to C++11 standard. PUBLIC:
# means any target linking against libcalclib.a also needs to be compiled under
# C++11 standards or higher.

# target_compile_features(calclib PUBLIC cxx_std_11)

# produces the executable named calc to be run via ./calc from /build folder
add_executable(run_sim src/main.cpp)

# NOTE: add this for testing
# add_executable(mem_test tests/FILENAME.cpp)
# target_include_directories(mem_test PRIVATE src)
# include(GoogleTest)
# target_link_libraries(mem_test PRIVATE GTest::gtest_main)
# gtest_discover_tests(mem_test)

# ensures that calc executable get's linked against libcalclib.a PRIVATE: almost
# redundant as nothing ever really links against an executable (calc) anyhow.

# target_link_libraries(calc PRIVATE calclib)
