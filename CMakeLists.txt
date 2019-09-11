cmake_minimum_required(VERSION 3.10)
set (PROJECT_NAME FEXCore)
project(${PROJECT_NAME}
  VERSION 0.01
  LANGUAGES CXX)

option(ENABLE_CLANG_FORMAT "Run clang format over the source" FALSE)
option(FORCE_AARCH64 "Force AArch64 Target for testing" FALSE)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(LLVM CONFIG QUIET)
if(LLVM_FOUND AND TARGET LLVM)
  message(STATUS "LLVM found!")
  include_directories(${LLVM_INCLUDE_DIRS})
else()
  message("Couldn't find LLVM and this project requires it")
endif()

include(CheckCXXCompilerFlag)
include(CheckIncludeFileCXX)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
  set(_M_X86_64 1)
  if (NOT FORCE_AARCH64)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-operator-names")
    set(CMAKE_REQUIRED_DEFINITIONS "-fno-operator-names")
    check_include_file_cxx(xbyak/xbyak.h XBYAK_FOUND)
    if (XBYAK_FOUND)
      set(ENABLE_JIT 1)
    else()
      message(STATUS "xbyak not found. Not enabling runtime JIT")
    endif()
  endif()
endif()
if (FORCE_AARCH64 OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
  set(_M_ARM_64 1)
  set(ENABLE_JIT 1)
  add_subdirectory(External/vixl/)
  include_directories(External/vixl/src/)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(External/SonicUtils/)
include_directories(External/SonicUtils/)

add_subdirectory(Source/)
target_include_directories(${PROJECT_NAME} PUBLIC include/)

add_subdirectory(Examples/)