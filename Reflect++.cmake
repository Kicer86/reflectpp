
function(ReflectFiles OutputVar)

    #get sources
    cmake_parse_arguments(ARG "" "TARGET" "SOURCES" ${ARGN} )

    foreach(source_file ${ARG_SOURCES})
        get_filename_component(source_name ${source_file} NAME_WE)
        set(output_name ${source_name}_r++.hpp)

        list(APPEND compiler_include_dirs ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
        list(TRANSFORM compiler_include_dirs PREPEND "-I")

        add_custom_command(
            OUTPUT ${output_name}
            COMMAND reflectpp ARGS ${CMAKE_CURRENT_BINARY_DIR}/${output_name} ${source_file} "$<LIST:TRANSFORM,$<TARGET_PROPERTY:${ARG_TARGET},INCLUDE_DIRECTORIES>,PREPEND,-I>" ${compiler_include_dirs} -std=c++${CMAKE_CXX_STANDARD}
            COMMAND_EXPAND_LISTS
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS ${source_file} reflectpp
        )

        list(APPEND ${OutputVar} ${CMAKE_CURRENT_BINARY_DIR}/${output_name})

    endforeach()

    set_source_files_properties(${${OutputVar}} PROPERTIES GENERATED TRUE)

    set(${OutputVar} ${${OutputVar}} PARENT_SCOPE)

endfunction()
