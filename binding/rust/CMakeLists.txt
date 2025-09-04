project(coralreefplayer-rs VERSION ${CoralReefPlayer_VERSION})

include(copy_libs)

find_program(CARGO_EXECUTABLE NAMES cargo)
if(NOT CARGO_EXECUTABLE)
    message(FATAL_ERROR "cargo not found")
endif()

file(
    COPY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

configure_file(
    ${CMAKE_CURRENT_BINARY_DIR}/Cargo.toml.in
    ${CMAKE_CURRENT_BINARY_DIR}/Cargo.toml
    @ONLY
)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/generate.cmake "
    configure_file(
        ${CMAKE_CURRENT_BINARY_DIR}/build.rs.in
        ${CMAKE_CURRENT_BINARY_DIR}/build.rs
        @ONLY
    )
")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/build.rs
    COMMAND ${CMAKE_COMMAND}
            -DCRP_LIBRARY_DIR="$<TARGET_FILE_DIR:CoralReefPlayer>"
            -DCRP_LIBRARY=CoralReefPlayer
            -P ${CMAKE_CURRENT_BINARY_DIR}/generate.cmake
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/build.rs.in
)

add_custom_target(rust-prepare
    COMMAND echo "Build rust binding"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS CoralReefPlayer
            ${CMAKE_CURRENT_BINARY_DIR}/build.rs
)
add_copy_libs_command(rust-prepare lib/)

add_custom_target(rust-package
    COMMAND cargo build
    # COMMAND cargo publish
    DEPENDS rust-prepare
)
