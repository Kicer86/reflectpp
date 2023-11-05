
find_package(GTest)

if(GTest_FOUND)

    include(../Reflect++.cmake)

    add_executable(reflectpp_test
        test_members_getting.cpp
        test_members_setting.cpp
        test_nested_types.cpp
    )

    ReflectFiles(
        ReflectedFiles
        TARGET
            reflectpp_test
        SOURCES
            struct_a.hpp
            struct_b.hpp
    )

    target_sources(reflectpp_test
        PRIVATE
            ${ReflectedFiles}
    )

    target_include_directories(reflectpp_test
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}
    )

    target_link_libraries(reflectpp_test
        PRIVATE
            GTest::gmock
            GTest::gmock_main
    )

    add_test(NAME reflectpp COMMAND reflectpp_test)

endif()
