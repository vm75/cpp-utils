# Tests need to be added as executables first
add_executable(testutil tiny-json_test.cpp)

# Set minimum c++ version
target_compile_features(testutil PRIVATE cxx_std_20)

# Should be linked to the main library
target_link_libraries(testutil PRIVATE cpp_utils)

# If you register a test, then ctest and make test will run it.
# You can also run examples and check the output, as well.
enable_testing()
add_test(NAME tinyJsonTests COMMAND testutil tiny-json) # Command can be a target
