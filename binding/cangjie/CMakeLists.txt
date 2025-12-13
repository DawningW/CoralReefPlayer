project(coralreefplayer_cj VERSION ${CoralReefPlayer_VERSION})

include(copy_libs)

find_program(CJPM_EXECUTABLE NAMES cjpm)
if(NOT CJPM_EXECUTABLE)
    message(FATAL_ERROR "cangjie not found")
endif()

file(
    COPY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/generate.cmake "
    configure_file(
        ${CMAKE_CURRENT_BINARY_DIR}/cjpm.toml.in
        ${CMAKE_CURRENT_BINARY_DIR}/cjpm.toml
        @ONLY
    )
")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cjpm.toml
    COMMAND ${CMAKE_COMMAND}
            -DPROJECT_VERSION=${PROJECT_VERSION}
            -DCRP_LIBRARY_DIR="$<TARGET_FILE_DIR:CoralReefPlayer>"
            -DCRP_LIBRARY=CoralReefPlayer
            -P ${CMAKE_CURRENT_BINARY_DIR}/generate.cmake
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/cjpm.toml.in
)

if(LINUX)
    target_compile_definitions(CoralReefPlayer
        PRIVATE
            CANGJIE_FIX
    )
endif()

add_custom_target(cangjie-prepare
    COMMAND echo "Build cangjie binding"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS CoralReefPlayer
            ${CMAKE_CURRENT_BINARY_DIR}/cjpm.toml
)
add_copy_libs_command(cangjie-prepare lib/)

add_custom_target(cangjie-package
    COMMAND cjpm build
    DEPENDS cangjie-prepare
)
