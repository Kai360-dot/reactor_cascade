cmake_minimum_required(VERSION 3.15...4.1)
project(reactor_cascade LANGUAGES CXX)


if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_library(magnus_stack INTERFACE)
target_include_directories(
  magnus_stack SYSTEM
  INTERFACE /opt/magnus/src
            /opt/canon/src
            /opt/cronos/src
            /opt/mcpp/src/mc
            /opt/mcpp/src/3rdparty/fadbad++
            /opt/mcpp/src/3rdparty/cpplapack-2015.05.11-1/include
            /opt/mcpp/src/3rdparty/boost
            /usr/include/suitesparse
            /opt/sundials-7.4.0/include
            /opt/snopt77/include
            /opt/gurobi1203/linux64/include
            /opt/gams/gams53.4_linux_x64_64_sfx/apifiles/C/api)

target_compile_definitions(
  magnus_stack
  INTERFACE MC__USE_THREAD
            MC__USE_TADIFF
            MC__USE_ARMADILLO
            BOOST_UBLAS_NO_STD_CERR
            MC__USE_BOOST
            MC__USE_HSL
            CRONOS__WITH_KLU
            MC__USE_SOBOL
            MC__USE_SNOPT
            MC__USE_GUROBI
            MC__WITH_GAMS="/opt/gams/gams53.4_linux_x64_64_sfx")

target_compile_features(magnus_stack INTERFACE cxx_std_17)
target_link_directories(magnus_stack INTERFACE /opt/sundials-7.4.0/lib
                        /opt/snopt77/lib /opt/gurobi1203/linux64/lib)
target_link_libraries(
  magnus_stack
  INTERFACE armadillo
            lapack
            blas
            mc13
            mc21
            mc33
            gfortran
            sundials_sunlinsolklu
            sundials_cvodes
            sundials_nvecserial
            sundials_core
            boost_program_options
            boost_random
            snopt7_cpp
            gurobi_c++
            gurobi120
            pthread)

set(CMAKE_BUILD_RPATH
    "/opt/sundials-7.4.0/lib;/opt/snopt77/lib;/opt/gurobi1203/linux64/lib")

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
add_executable(global_sample src/global_sample.cpp)

target_link_libraries(global_sample PRIVATE magnus_stack)

# Sobol sweep of joint design box -> cstr-2 inlet box
add_executable(inlet_box_sobol src/inlet_box_sobol.cpp)
target_link_libraries(inlet_box_sobol PRIVATE magnus_stack)

add_executable(unit1_sample src/unit1_sample.cpp)
target_link_libraries(unit1_sample PRIVATE magnus_stack)

add_executable(unit2_sample src/unit2_sample.cpp)
target_link_libraries(unit2_sample PRIVATE magnus_stack)

add_executable(mlp_check src/mlp_check.cpp)
target_link_libraries(mlp_check PRIVATE magnus_stack)

add_executable(reconstruct src/reconstruct.cpp)
target_link_libraries(reconstruct PRIVATE magnus_stack)

# NOTE: add this for testing
add_executable(reactor_tests tests/flowsheet_test.cpp)
target_include_directories(reactor_tests PRIVATE tests src)
include(GoogleTest)
target_link_libraries(reactor_tests PRIVATE magnus_stack GTest::gtest_main)
gtest_discover_tests(reactor_tests)

# ensures that calc executable get's linked against libcalclib.a PRIVATE: almost
# redundant as nothing ever really links against an executable (calc) anyhow.

# target_link_libraries(calc PRIVATE calclib)
