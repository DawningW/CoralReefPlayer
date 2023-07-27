cmake_minimum_required(VERSION 3.16.0)

project(CoralReefCamCpp)

if(MSVC)
    if(NOT CMAKE_VS_PLATFORM_NAME)
        set(CMAKE_VS_PLATFORM_NAME x64 CACHE STRING "" FORCE)
    endif()
    if(NOT ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64"))
        message(FATAL_ERROR "${CMAKE_VS_PLATFORM_NAME} arch is not supported!")
    endif()
    message("${CMAKE_VS_PLATFORM_NAME} architecture in use")
endif(MSVC)

set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING "" FORCE)

add_subdirectory(live555)
add_subdirectory(ffmpeg)
add_subdirectory(CoralReefPlayer)
add_subdirectory(CoralReefCam)
