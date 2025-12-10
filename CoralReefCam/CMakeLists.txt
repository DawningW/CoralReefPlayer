project(CoralReefCam)

set(CMAKE_CXX_STANDARD 20)

if(WIN32)
    set(SDL2_DIR "${THIRD_PARTY_DIR}/SDL2/cmake" CACHE PATH "")
    find_package(SDL2 2.28.0 REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS SDL2::SDL2
        LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    )
elseif(EMSCRIPTEN)
    set(SDL2_DIR "${EMSCRIPTEN_ROOT_PATH}/tools/ports/sdl2" CACHE PATH "")
    find_package(SDL2 REQUIRED)
    set(EMRUN "${EMSCRIPTEN_ROOT_PATH}/emrun${EMCC_SUFFIX}")
else()
    find_package(SDL2 2.0.18 REQUIRED)
endif()

find_package(OpenCV 4.4.0 QUIET)
set(ENABLE_OPENCV ON CACHE BOOL "")
if(NOT OpenCV_FOUND)
    message("OpenCV not found, define ENABLE_OPENCV with 0")
    set(ENABLE_OPENCV OFF CACHE BOOL "" FORCE)
endif()
if(WIN32 AND ENABLE_OPENCV)
    install(IMPORTED_RUNTIME_ARTIFACTS ${OpenCV_LIBS}
        LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    )
endif()

add_subdirectory(${THIRD_PARTY_DIR}/imgui imgui)
add_subdirectory(${THIRD_PARTY_DIR}/implot implot)

if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
endif()
if(APPLE AND BUILD_MACOS_FRAMEWORK AND BUILD_MACOS_BUNDLE)
    list(APPEND CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")
else()
    list(APPEND CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})
endif()

add_executable(${PROJECT_NAME}
    main.cpp
    $<$<OR:$<BOOL:${WIN32}>,$<BOOL:${CYGWIN}>>:CoralReefCam.rc>
    $<$<BOOL:${APPLE}>:icon.icns>
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/.
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        ENABLE_OPENCV=$<BOOL:${ENABLE_OPENCV}>
)

if(EMSCRIPTEN)
    target_link_options(${PROJECT_NAME}
        PRIVATE
            $<$<BOOL:${USE_PTHREAD}>:-sPTHREAD_POOL_SIZE=3>
            -sASYNCIFY=1
            -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        CoralReefPlayer
        SDL2::SDL2
        imgui
        implot
        $<$<BOOL:${ENABLE_OPENCV}>:${OpenCV_LIBS}>
)

add_dependencies(${PROJECT_NAME}
    CoralReefPlayer
    SDL2::SDL2
    imgui
    implot
)
if(ENABLE_OPENCV)
    add_dependencies(${PROJECT_NAME}
        ${OpenCV_LIBS}
    )
endif()

if(APPLE AND BUILD_MACOS_BUNDLE)
    set_target_properties(${PROJECT_NAME}
        PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
            RESOURCE ${CMAKE_CURRENT_SOURCE_DIR}/icon.icns
    )
elseif(EMSCRIPTEN)
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "index")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    add_custom_target(preview
        COMMAND ${EMRUN} index.html
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

if (NOT EMSCRIPTEN)
    install(TARGETS ${PROJECT_NAME}
        DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    )
else()
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/index.html
        ${CMAKE_CURRENT_BINARY_DIR}/index.js
        ${CMAKE_CURRENT_BINARY_DIR}/index.wasm
        DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    )
endif()
