# Copyright 2021, Collabora, Ltd.
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.2)
project(jnipp)

find_package(JNI REQUIRED)
include(CTest)

option(
    JNIPP_USE_EXCEPTION_HANDLING
    "Enable exception handling in JNIPP."
    ON
)

add_library(jnipp jnipp.cpp)

if(NOT JNIPP_USE_EXCEPTION_HANDLING)
  target_compile_definitions(jnipp PUBLIC JNIPP_USE_EXCEPTION=0)

  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(jnipp PRIVATE -fno-exceptions)
  endif()
else()
  target_compile_definitions(jnipp PUBLIC JNIPP_USE_EXCEPTION=1)
endif()

target_include_directories(
  jnipp
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${JNI_INCLUDE_DIRS})
target_compile_features(jnipp PUBLIC cxx_std_11)
target_link_libraries(jnipp PUBLIC ${CMAKE_DL_LIBS})

add_subdirectory(tests)
