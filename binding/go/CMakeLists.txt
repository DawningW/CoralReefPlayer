project(gcrp VERSION ${CoralReefPlayer_VERSION})

include(copy_libs)

find_program(GO_EXECUTABLE NAMES go)
if(NOT GO_EXECUTABLE)
    message(FATAL_ERROR "Go binding needs go >= 1.18 to build!")
    return()
endif()

get_target_property(CRP_INCLUDE_DIR CoralReefPlayer INTERFACE_INCLUDE_DIRECTORIES)
list(GET CRP_INCLUDE_DIR 1 CRP_HEADER_DIR)

file(
    COPY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(go-prepare
    COMMAND echo "Build go binding"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS CoralReefPlayer
)
add_custom_command(
    TARGET go-prepare POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CRP_HEADER_DIR}/coralreefplayer.h gcrp/
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
add_copy_libs_command(go-prepare gcrp/)

add_custom_target(go-package
    COMMAND ${GO_EXECUTABLE} build -buildmode=archive .
    COMMAND ${CMAKE_COMMAND} -E tar cfvz ${CMAKE_INSTALL_PREFIX}/gcrp-${PROJECT_VERSION}.tar.gz ../gcrp
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gcrp
    DEPENDS go-prepare
)
