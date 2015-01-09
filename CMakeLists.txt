cmake_minimum_required(VERSION 2.8.11)
project(jrimage)
set(CMAKE_VERBOSE_MAKEFILE off)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# TODO(cbraley): Figure out how to examine ASM.

# Create source file lists. ---------------------------------------------------

# Source files for jrimage (excluding tests, benchmarks, and files w/ main()).
set(SRC_FILES src/jrimage.cc src/mem_utils.cc src/jrimage_color.cc)
file(GLOB TEST_SRCS tests/*tests.cc)  # Src files containing tests.
file(GLOB BENCHMARK_SRCS benchmarks/*benchmarks.cc)  # Src files w/ benchmarks.

# Source files containing each main(...) function.
set(MAIN_SRC_UNIT_TESTS tests/main_unit_tests.cc)
set(MAIN_SRC_BENCHMARKS benchmarks/main_benchmarks.cc)

# Determine build configuration and set compiler flags. -----------------------

# Make sure we are either in release or debug build configuration.  We create
# variable BTAG with a string value that represents the build configuration.
set(BTAG "")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  # TODO(cbraley): Case-insensitive comparison here.
  message(STATUS "DEBUG mode build.")
  set(BTAG "debug")
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "RELEASE mode build.")
  set(BTAG "release")
else()
  message(STATUS "Unrecognized or unset build type.")
  message(STATUS "Possible build types include {Release, Debug}.")
  message(STATUS "To build in mode X, invoke cmake like: \"cmake -DCMAKE_BUILD_TYPE=X ..\".")
  message(STATUS "where X is one of the available build types: {Release, Debug}.")
  message(STATUS "Defaulting to Debug mode.")
  set(BTAG "debug")
  set(CMAKE_BUILD_TYPE Debug)
endif()

#  Set mode-dependent compiler flags.
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG -S")
set(CMAKE_CXX_FLAGS_RELEASE "-fno-strict-aliasing -O3 -DNDEBUG")

# Set compiler flags for each build confg.
set(DONT_WARN_ON_UNUSED "-Wno-unused-variable -Wno-unused-parameter -Wno-unused-function")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic ${DONT_WARN_ON_UNUSED}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fshow-source-location -fcaret-diagnostics")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-format=vi")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -DDEBUG")

# TODO(cbraley): Turn back on address sanitizer once Apple ships an updated Clang.
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=alignment")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=bounds")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=float-divide-by-zero")

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -funroll-loops -march=native -msse3 -DNDEBUG")

# Dependencies. ---------------------------------------------------------------

# gtest unit test library.
set(GTEST_DIR "third_party/gtest-1.7.0")
add_subdirectory("${GTEST_DIR}")
set(GTEST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/${GTEST_DIR}")
set(GTEST_INCLUDE_DIR "${GTEST_ROOT}/include")
set(GTEST_LIBRARIES gtest gtest_main)
include_directories(${GTEST_INCLUDE_DIR})

# gbenchmark benchmarking library.
set(GBENCH_DIR "third_party/gbenchmark")
add_subdirectory("${GBENCH_DIR}")
set(GBENCH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/${GBENCH_DIR}")
set(GBENCH_INCLUDE_DIR "${GBENCH_ROOT}/include")
set(GBENCH_LIBRARIES benchmark)
include_directories(${GBENCH_INCLUDE_DIR})

# Add phtreads for benchmarking lib.
find_package(Threads)

# Add each individual executable. ---------------------------------------------
SET(UNIT_TESTS_BINARY jrimage_unit_tests_${BTAG})
SET(BENCHMARKS_BINARY jrimage_benchmarks_${BTAG})

add_executable(${UNIT_TESTS_BINARY} ${SRC_FILES} ${TEST_SRCS}      ${MAIN_SRC_UNIT_TESTS})
add_executable(${BENCHMARKS_BINARY} ${SRC_FILES} ${BENCHMARK_SRCS} ${MAIN_SRC_BENCHMARKS})

# Set directories to search for header files. ---------------------------------

# Note that we search src/ as well since we have some private header
# files (only public header files are placed inside include/).
set(INCLUDE_DIRS "include src")

target_include_directories(${UNIT_TESTS_BINARY} PRIVATE include src tests)
target_include_directories(${BENCHMARKS_BINARY} PRIVATE include src benchmarks)

# Linking. --------------------------------------------------------------------
target_link_libraries(${UNIT_TESTS_BINARY} ${GTEST_LIBRARIES})
target_link_libraries(${BENCHMARKS_BINARY} ${GBENCH_LIBRARIES})
target_link_libraries(${BENCHMARKS_BINARY} ${CMAKE_THREAD_LIBS_INIT})

