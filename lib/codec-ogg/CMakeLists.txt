cmake_minimum_required(VERSION 3.16)

# set the project name
project(arduino_libogg)

# Set default build type to Release (with optimization)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# lots of warnings and all warnings as errors
## add_compile_options(-Wall -Wextra )
set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRC_LIST_C CONFIGURE_DEPENDS  "${PROJECT_SOURCE_DIR}/src/*.c" )

# define libraries
add_library (arduino_libogg ${SRC_LIST_C})

# prevent compile errors and enable optimization
target_compile_options(arduino_libogg PRIVATE -DARDUINO -DHAVE_CONFIG_H -Wno-stringop-overread -O2)

# define location for header files
target_include_directories(arduino_libogg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src  )