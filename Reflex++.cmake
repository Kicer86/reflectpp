
function(ReflexFiles output source_files)

    foreach(source_file ${source_files})
        get_filename_component(source_name ${source_file} NAME_WE)
        set(output_name ${source_name}_r++.hpp)

        add_custom_command(
            OUTPUT ${output_name}
            COMMAND reflexpp ARGS ${source_file} > ${CMAKE_CURRENT_BINARY_DIR}/${output_name}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS ${source_file} reflexpp
        )

        list(APPEND ${output} ${CMAKE_CURRENT_BINARY_DIR}/${output_name})

    endforeach()

    set_source_files_properties(${${output}} PROPERTIES GENERATED TRUE)

    set(${output} ${${output}} PARENT_SCOPE)

endfunction()
