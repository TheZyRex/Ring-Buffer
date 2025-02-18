cmake_minimum_required(VERSION 3.10)

# Set the project name
project(RingBufferProject)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(${PROJECT_SOURCE_DIR})

# Add the source files
set(SOURCES
    ringbuffer.c
)

# Add the header files
set(HEADERS
    ringbuffer.h
)

# Create the library
add_library(ringbuffer STATIC ${SOURCES} ${HEADERS})

# Add an executable for testing (optional)
add_executable(test_ringbuffer test_ringbuffer.cpp)

# Link the library to the executable
target_link_libraries(test_ringbuffer ringbuffer)