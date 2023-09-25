
cmake_minimum_required(VERSION 3.25)
project(reflexpp LANGUAGES C CXX)

find_package(Clang REQUIRED 16)

add_subdirectory(tests)

# if Config mode was used (linux and macos) libclang target will be available
# on windows, in most cases Module mode will be used which sets CLANG_LIBRARIES
if(TARGET libclang AND NOT CLANG_LIBRARIES)
    set(CLANG_LIBRARIES libclang)
endif()

add_executable(reflectpp
    main.cpp
)

target_include_directories(reflectpp PRIVATE ${CLANG_INCLUDE_DIRS})
target_link_libraries(reflectpp PRIVATE ${CLANG_LIBRARIES})
