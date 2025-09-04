project(pyCoralReefPlayer VERSION ${CoralReefPlayer_VERSION})

include(copy_libs)

find_package(Python3 3.7 COMPONENTS Interpreter Development)
if(NOT Python3_FOUND)
    message(FATAL_ERROR "Python binding needs Python >= 3.7 to build!")
    return()
endif()

get_target_property(CRP_INCLUDE_DIR CoralReefPlayer INTERFACE_INCLUDE_DIRECTORIES)
string(REPLACE ";" "\\;" CRP_INCLUDE_DIR "${CRP_INCLUDE_DIR}")

file(
    COPY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/generate.cmake "
    string(REPLACE \";\" \"\\\",\\n\\\"\" CRP_INCLUDE_DIR \"\${CRP_INCLUDE_DIR}\")
    configure_file(
        ${CMAKE_CURRENT_BINARY_DIR}/setup.py.in
        ${CMAKE_CURRENT_BINARY_DIR}/setup.py
        @ONLY
    )
")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/setup.py
    COMMAND ${CMAKE_COMMAND}
            -DPROJECT_VERSION="${PROJECT_VERSION}"
            -DCRP_INCLUDE_DIR="${CRP_INCLUDE_DIR}"
            -DCRP_LIBRARY_DIR="$<TARGET_FILE_DIR:CoralReefPlayer>"
            -DCRP_LIBRARY=CoralReefPlayer
            -P ${CMAKE_CURRENT_BINARY_DIR}/generate.cmake
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/setup.py.in
)

add_custom_target(python-prepare
    COMMAND echo "Build python binding"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS CoralReefPlayer
            ${CMAKE_CURRENT_BINARY_DIR}/setup.py
)
add_copy_libs_command(python-prepare coralreefplayer/)

add_custom_target(python-package
    COMMAND ${Python3_EXECUTABLE} setup.py bdist_wheel --dist-dir ${CMAKE_INSTALL_PREFIX}
    DEPENDS python-prepare
)
