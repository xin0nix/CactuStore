cmake_minimum_required(VERSION 3.20) 

project(CactuStore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CactuStore
    Coro.cpp
    EventLoop.cpp
    IpAddress.cpp
    TcpSocket.cpp
    Timer.cpp
    main.cpp
) 

target_compile_options(CactuStore PRIVATE -fsanitize=thread)
target_link_options(CactuStore PRIVATE -fsanitize=thread)
