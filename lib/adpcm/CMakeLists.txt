cmake_minimum_required(VERSION 3.16)

# set the project name
project(adpcm_ffmpeg)

# lots of warnings and all warnings as errors
# add_compile_options(-Wall -Wextra )

# define location for header files
add_subdirectory("src")
add_subdirectory("tests/sine")

