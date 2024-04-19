project(coralreefplayer-js VERSION ${CoralReefPlayer_VERSION})

include(copy_libs)

find_program(NodeJS_EXECUTABLE NAMES node)
if(NOT NodeJS_EXECUTABLE)
    message(FATAL_ERROR "JavaScript binding needs Node.js >= 12 to build!")
    return()
endif()

get_target_property(CRP_INCLUDE_DIR CoralReefPlayer INTERFACE_INCLUDE_DIRECTORIES)
string(REPLACE ";" "',\n'" CRP_INCLUDE_DIR "${CRP_INCLUDE_DIR}")
get_target_property(CRP_LIBRARY_DIR CoralReefPlayer BINARY_DIR)
set(CRP_LIBRARY CoralReefPlayer)

file(
    COPY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

configure_file(
    ${CMAKE_CURRENT_BINARY_DIR}/package.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/package.json
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_BINARY_DIR}/binding.gyp.in
    ${CMAKE_CURRENT_BINARY_DIR}/binding.gyp
    @ONLY
)

add_custom_target(node-prepare
    COMMAND echo "Build nodejs binding"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS CoralReefPlayer
)
add_copy_libs_command(node-prepare prebuilds/)

add_custom_target(node-package
    COMMAND npm install
    COMMAND npm run prebuild
    COMMAND npm run package -- --pack-destination ${CMAKE_INSTALL_PREFIX}
    DEPENDS node-prepare
)
