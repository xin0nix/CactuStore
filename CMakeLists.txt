cmake_minimum_required(VERSION 3.20)  # Minimum CMake version supporting C++23 standard setting

project(CactuStore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)            # Set the C++ standard to C++23
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Require the standard (error if not supported)

add_executable(CactuStore main.cpp)  # Create executable from main.cpp
