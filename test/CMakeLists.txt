
add_executable(test_start test_start.cpp)
target_link_libraries(test_start
  PRIVATE
    rust_cpp_iterators
)
add_test(test_start test_start)
