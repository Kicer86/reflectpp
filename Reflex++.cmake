
function(ReflexFiles target output)

    foreach(source_file ${ARGN})
        get_filename_component(source_name ${source_file} NAME_WE)
        set(output_name ${source_name}_r++.hpp)

        add_custom_command(
            OUTPUT ${output_name}
            COMMAND reflexpp ARGS ${CMAKE_CURRENT_BINARY_DIR}/${output_name} ${source_file} "$<LIST:TRANSFORM,$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>,PREPEND,-I>"
            COMMAND_EXPAND_LISTS
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS ${source_file} reflexpp
        )

        list(APPEND ${output} ${CMAKE_CURRENT_BINARY_DIR}/${output_name})

    endforeach()

    set_source_files_properties(${${output}} PROPERTIES GENERATED TRUE)

    set(${output} ${${output}} PARENT_SCOPE)

endfunction()
