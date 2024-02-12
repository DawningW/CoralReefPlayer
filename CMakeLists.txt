cmake_minimum_required(VERSION 3.21.0)

# Disable in-source builds to prevent source tree corruption.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed. You should create a separate directory for build files.")
endif()

project(CoralReefPlayer VERSION 1.0)

set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING "" FORCE)
if(MSVC)
    if(NOT CMAKE_VS_PLATFORM_NAME)
        set(CMAKE_VS_PLATFORM_NAME x64 CACHE STRING "" FORCE)
    endif()
    if(NOT ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64"))
        message(FATAL_ERROR "${CMAKE_VS_PLATFORM_NAME} arch is not supported!")
    endif()
    message(STATUS "Architecture: ${CMAKE_VS_PLATFORM_NAME}")

    set(LINK_STATIC_MSVC_LIBS OFF CACHE BOOL "")
    if(LINK_STATIC_MSVC_LIBS)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    
    add_compile_options("/utf-8")
endif()

set(CMAKE_MODULE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_MODULE_DIR})
set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
if(WIN32 AND NOT CYGWIN)
    set(CMAKE_INSTALL_LIBDIR "CoralReefCam" CACHE PATH "")
    set(CMAKE_INSTALL_BINDIR "CoralReefCam" CACHE PATH "")
    set(CMAKE_INSTALL_INCLUDEDIR "CoralReefCam" CACHE PATH "")
endif()
include(GNUInstallDirs)
foreach(dir LIB BIN INCLUDE)
    message(STATUS "Installing ${dir} components to ${CMAKE_INSTALL_FULL_${dir}DIR}")
endforeach()

set(CURL_USE_STATIC_LIBS OFF CACHE BOOL "")
if(WIN32)
    set(CURL_DIR "${THIRD_PARTY_DIR}/libcurl/lib/cmake/CURL" CACHE PATH "")
    find_package(CURL REQUIRED)
    if(NOT CURL_USE_STATIC_LIBS)
        set(CURL_LIBS CURL::libcurl_shared)
    else()
        set(CURL_LIBS CURL::libcurl_static)
    endif()
    install(IMPORTED_RUNTIME_ARTIFACTS ${CURL_LIBS}
        LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    )
else()
    find_package(CURL REQUIRED)
    set(CURL_LIBS CURL::libcurl)
endif()

add_subdirectory(${THIRD_PARTY_DIR}/live555)
add_subdirectory(${THIRD_PARTY_DIR}/ffmpeg)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(binding)
set(BUILD_DEMO ON CACHE BOOL "Build CoralReefCam")
if(BUILD_DEMO)
    add_subdirectory(CoralReefCam)
endif()
