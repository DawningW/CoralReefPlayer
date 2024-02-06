macro(add_copy_libs_command target dest)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${dest})
    add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:CoralReefPlayer> ${dest}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    if(WIN32 OR CYGWIN)
        get_target_property(CRP_DEPENDENCIES CoralReefPlayer LINK_LIBRARIES)
        foreach(DEP IN LISTS CRP_DEPENDENCIES)
            get_target_property(DEP_TYPE ${DEP} TYPE)
            if(NOT DEP_TYPE STREQUAL "SHARED_LIBRARY")
                continue()
            endif()
            add_custom_command(
                TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${DEP}> ${dest}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            )
        endforeach()
    endif()
endmacro()
