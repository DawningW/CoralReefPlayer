cmake_minimum_required(VERSION 3.21)
project(coralreefplayer VERSION 1.4.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(NOT DEFINED CRP_ROOT OR NOT EXISTS "${CRP_ROOT}/src")
    message(FATAL_ERROR "CRP_ROOT is not set correctly.")
endif()
if(NOT DEFINED CRP_LIVE555_DIR OR NOT EXISTS "${CRP_LIVE555_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "CRP_LIVE555_DIR is not set correctly.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory("${CRP_LIVE555_DIR}" live555)

find_package(httplib CONFIG REQUIRED)
find_package(FFMPEG REQUIRED)
if(NOT FFMPEG_LIBRARIES)
    message(FATAL_ERROR "FFMPEG_LIBRARIES is empty, cannot link FFmpeg.")
endif()
if(NOT TARGET httplib::httplib)
    message(FATAL_ERROR "Target httplib::httplib is required but was not found.")
endif()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")

execute_process(
    COMMAND git describe --abbrev=7 --dirty --always
    WORKING_DIRECTORY "${CRP_ROOT}"
    OUTPUT_VARIABLE _crp_git_hash
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT _crp_git_hash)
    set(_crp_git_hash "unknown")
endif()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated/git_hash.h" "#define CRP_GIT_HASH \"${_crp_git_hash}\"\n")

set(CRP_SHARED_LIB 1)
set(CoralReefPlayer_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CoralReefPlayer_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CoralReefPlayer_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CoralReefPlayer_VERSION_TWEAK 0)
set(CoralReefPlayer_VERSION ${PROJECT_VERSION})

configure_file("${CRP_ROOT}/src/version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/version.h" @ONLY)
configure_file("${CRP_ROOT}/src/coralreefplayer.h.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/coralreefplayer.h" @ONLY)

add_library(CoralReefPlayer SHARED
    "${CMAKE_CURRENT_BINARY_DIR}/generated/coralreefplayer.h"
    "${CRP_ROOT}/src/dllmain.cpp"
    "${CRP_ROOT}/src/AsyncCallback.hpp"
    "${CRP_ROOT}/src/Matcher.hpp"
    "${CRP_ROOT}/src/StreamPuller.cpp"
    "${CRP_ROOT}/src/StreamSink.cpp"
    "${CRP_ROOT}/src/VideoDecoder.cpp"
    "${CRP_ROOT}/src/AudioDecoder.cpp"
    $<$<PLATFORM_ID:Windows>:${CRP_ROOT}/src/CoralReefPlayer.rc>
)

target_include_directories(CoralReefPlayer
    PUBLIC
        $<BUILD_INTERFACE:${CRP_ROOT}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${FFMPEG_INCLUDE_DIRS}
)

target_compile_definitions(CoralReefPlayer PRIVATE CORALREEFPLAYER_EXPORTS)
target_link_libraries(CoralReefPlayer
    PRIVATE
        live555
        httplib::httplib
        ${FFMPEG_LIBRARIES}
)

set_target_properties(CoralReefPlayer PROPERTIES
    OUTPUT_NAME "CoralReefPlayer"
    EXPORT_NAME coralreefplayer
)

add_library(coralreefplayer::coralreefplayer ALIAS CoralReefPlayer)

install(TARGETS CoralReefPlayer
    EXPORT coralreefplayerTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/coralreefplayer.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(EXPORT coralreefplayerTargets
    NAMESPACE coralreefplayer::
    DESTINATION "share/coralreefplayer"
)

configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/coralreefplayerConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/coralreefplayerConfig.cmake"
    INSTALL_DESTINATION "share/coralreefplayer"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/coralreefplayerConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/coralreefplayerConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/coralreefplayerConfigVersion.cmake"
    DESTINATION "share/coralreefplayer"
)
