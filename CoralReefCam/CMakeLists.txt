project(CoralReefCam)

if(WIN32)
    set(SDL2_DIR "${CMAKE_SOURCE_DIR}/SDL2/cmake" CACHE PATH "")
    find_package(SDL2 2.28.0 REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS SDL2::SDL2
        LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    )
else()
    find_package(SDL2 2.28.0 REQUIRED)
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

add_subdirectory(${CMAKE_SOURCE_DIR}/imgui imgui)
add_subdirectory(${CMAKE_SOURCE_DIR}/implot implot)

add_executable(${PROJECT_NAME}
    main.cpp
    $<$<OR:$<BOOL:${WIN32}>,$<BOOL:${CYGWIN}>>:CoralReefCam.rc>
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/.
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        ENABLE_OPENCV=$<BOOL:${ENABLE_OPENCV}>
)

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

install(TARGETS ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
)

install(FILES unifont-15.0.06.ttf
    DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
)
