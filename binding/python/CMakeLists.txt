project(pyCoralReefPlayer VERSION ${CoralReefPlayer_VERSION})

include(copy_libs)

find_package(Python3 3.7 COMPONENTS Interpreter Development)
if(NOT Python3_FOUND)
    message(FATAL_ERROR "Python binding needs Python >= 3.7 to build!")
    return()
endif()

get_target_property(CRP_INCLUDE_DIR CoralReefPlayer INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(CRP_LIBRARY_DIR CoralReefPlayer BINARY_DIR)
set(CRP_LIBRARY CoralReefPlayer)

file(
    COPY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

configure_file(
    ${CMAKE_CURRENT_BINARY_DIR}/setup.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/setup.py
    @ONLY
)

add_custom_target(python-prepare
    COMMAND echo "Build python binding"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS CoralReefPlayer
)
add_copy_libs_command(python-prepare coralreefplayer/)

add_custom_target(python-package
    COMMAND ${Python3_EXECUTABLE} setup.py bdist_wheel --dist-dir ${CMAKE_INSTALL_PREFIX}
    DEPENDS python-prepare
)
