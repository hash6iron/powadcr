cmake_minimum_required(VERSION 3.16)

# set the project name
project(arduino_liblame)

option(BUILD_TESTS "Build all tests automatically" OFF)

# lots of warnings and all warnings as errors
## add_compile_options(-Wall -Wextra )
set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRC_LIST_C CONFIGURE_DEPENDS  "${PROJECT_SOURCE_DIR}/src/*.c" "${PROJECT_SOURCE_DIR}/src/*.cpp" )

# define libraries
add_library (arduino_liblame ${SRC_LIST_C})

# prevent compile errors
target_compile_options(arduino_liblame PRIVATE -DUSE_DEFAULT_STDLIB)

# define location for header files
target_include_directories(arduino_liblame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src  )

# build examples
if (BUILD_TESTS)
    add_subdirectory( "${CMAKE_CURRENT_SOURCE_DIR}/examples/encode")
    add_subdirectory( "${CMAKE_CURRENT_SOURCE_DIR}/examples/speed_test")
endif()