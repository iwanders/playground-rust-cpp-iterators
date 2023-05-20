cmake_minimum_required(VERSION 3.5.0)

project(RustCppIterators VERSION 0.0.1)

add_library(rust_cpp_iterators INTERFACE)

target_compile_features(rust_cpp_iterators INTERFACE cxx_std_20)

target_include_directories(rust_cpp_iterators
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

include(CTest)
if(BUILD_TESTING)
  enable_testing()
endif()

add_subdirectory(test)



file(GLOB_RECURSE FORMAT_SRC_FILES  "${PROJECT_SOURCE_DIR}/**.hpp"  "${PROJECT_SOURCE_DIR}/**.cpp")
add_custom_target(clang_format COMMAND clang-format -i ${FORMAT_SRC_FILES})

