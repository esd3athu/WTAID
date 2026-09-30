cmake_minimum_required(VERSION 3.20)
project(WTAID LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# WinHTTP library
target_link_libraries(WTAID PRIVATE winhttp)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

# Sources
set(SOURCES
    src/main.cpp
    src/network/HTTPClient.cpp
    src/ui/DataOutputWindow.cpp
)

add_executable(WTAID ${SOURCES})
