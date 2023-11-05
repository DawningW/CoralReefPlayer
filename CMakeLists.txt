cmake_minimum_required(VERSION 3.16.0)

# Disable in-source builds to prevent source tree corruption.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed. You should create a separate directory for build files.")
endif()

project(CoralReefCamCpp)

if(MSVC)
    set(LINK_STATIC_MSVC_LIB OFF CACHE BOOL "")
    if(LINK_STATIC_MSVC_LIB)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    if(NOT CMAKE_VS_PLATFORM_NAME)
        set(CMAKE_VS_PLATFORM_NAME x64 CACHE STRING "" FORCE)
    endif()
    if(NOT ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64"))
        message(FATAL_ERROR "${CMAKE_VS_PLATFORM_NAME} arch is not supported!")
    endif()
    message("${CMAKE_VS_PLATFORM_NAME} architecture in use")
endif()

set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING "" FORCE)

add_subdirectory(live555)
add_subdirectory(ffmpeg)
add_subdirectory(CoralReefPlayer)
add_subdirectory(CoralReefCam)
