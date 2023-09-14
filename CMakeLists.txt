
cmake_minimum_required(VERSION 3.27)
project(reflexpp LANGUAGES C CXX)

find_package(Clang REQUIRED)

add_executable(reflexpp
    main.cpp
)

target_link_libraries(reflexpp PRIVATE libclang)
