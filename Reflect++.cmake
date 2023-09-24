
function(ReflectFiles target output)

    foreach(source_file ${ARGN})
        get_filename_component(source_name ${source_file} NAME_WE)
        set(output_name ${source_name}_r++.hpp)

        list(APPEND compiler_include_dirs ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
        list(TRANSFORM compiler_include_dirs PREPEND "-I")

        add_custom_command(
            OUTPUT ${output_name}
            COMMAND reflectpp ARGS ${CMAKE_CURRENT_BINARY_DIR}/${output_name} ${source_file} "$<LIST:TRANSFORM,$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>,PREPEND,-I>" ${compiler_include_dirs} -std=c++${CMAKE_CXX_STANDARD}
            COMMAND_EXPAND_LISTS
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS ${source_file} reflectpp
        )

        list(APPEND ${output} ${CMAKE_CURRENT_BINARY_DIR}/${output_name})

    endforeach()

    set_source_files_properties(${${output}} PROPERTIES GENERATED TRUE)

    set(${output} ${${output}} PARENT_SCOPE)

endfunction()
