
function(ReflexFiles target output)

    set(includes)

    get_target_property(targets ${target} LINK_LIBRARIES)
    list(APPEND targets ${target})

    foreach(target ${targets})
        if(TARGET ${target})
            get_target_property(target_includes ${target} INCLUDE_DIRECTORIES)
            list(APPEND includes ${target_includes})
        endif()
    endforeach()
    list(SORT includes)
    list(REMOVE_DUPLICATES includes)

    list(TRANSFORM includes PREPEND "-I")
    list(JOIN includes ":" reflex_options)

    foreach(source_file ${ARGN})
        get_filename_component(source_name ${source_file} NAME_WE)
        set(output_name ${source_name}_r++.hpp)

        add_custom_command(
            OUTPUT ${output_name}
            COMMAND reflexpp ARGS ${source_file} ${reflex_options}> ${CMAKE_CURRENT_BINARY_DIR}/${output_name}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS ${source_file} reflexpp
        )

        list(APPEND ${output} ${CMAKE_CURRENT_BINARY_DIR}/${output_name})

    endforeach()

    set_source_files_properties(${${output}} PROPERTIES GENERATED TRUE)

    set(${output} ${${output}} PARENT_SCOPE)

endfunction()
