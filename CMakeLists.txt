cmake_minimum_required(VERSION 2.8)

project(HSIFileReader)

find_package(OpenCV)
IF(${OpenCV_FOUND})
  include_directories(${OpenCV_INCLUDE_DIRS})
  include_directories(${EIGEN_INCLUDE_DIR})
ENDIF()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Default to Release mode.
IF(NOT DEFINED CMAKE_BUILD_TYPE)
  SET(${CMAKE_BUILD_TYPE} Release ... FORCE)
ENDIF()

# Release mode makes the code run faster.
IF(NOT ${CMAKE_BUILD_TYPE} MATCHES Debug)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -Ofast")
ENDIF()

# Require C++ 11.
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Libraries will be stored in the "lib" directory, and binaries in "bin".
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Grab all binary source files in the src directory.
file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*")

# Add the test binary.
add_executable(
  HSIFileReaderTest
  src/hsi_data_reader.cpp
  src/test_reader.cpp
)

# Add visualization test binary if OpenCV is available.
IF(${OpenCV_FOUND})
  MESSAGE("Found OpenCV: Building Visualize binary as well.")
  add_executable(
    Visualize
    src/hsi_data_reader.cpp
    src/visualize.cpp
  )
  target_link_libraries(
    Visualize
    ${OpenCV_LIBS}
  )
ENDIF()
