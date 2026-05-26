
# build executable
add_executable (test test.cpp)

# find SinGenerator.h
target_include_directories(test PUBLIC ${PROJECT_SOURCE_DIR}/src )
target_compile_options    (test PUBLIC "-g3"  )

# add library
target_link_libraries(test PUBLIC adpcm)
