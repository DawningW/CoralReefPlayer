project(CoralReefCam)

find_package(SDL2 2.28.0 REQUIRED)
find_package(OpenCV 4.4.0)

add_subdirectory(../imgui imgui)

set(ENABLE_OPENCV ON CACHE BOOL "")
if(NOT OpenCV_FOUND)
    message("OpenCV not found, define ENABLE_OPENCV with 0")
    set(ENABLE_OPENCV OFF CACHE BOOL "" FORCE)
endif(NOT OpenCV_FOUND)

add_executable(${PROJECT_NAME}
    main.cpp
    $<$<BOOL:${WIN32}>:CoralReefCam.rc>
)

target_include_directories(${PROJECT_NAME}
PRIVATE
    ${PROJECT_SOURCE_DIR}/include
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
    $<$<BOOL:${ENABLE_OPENCV}>:opencv_world>
)

add_dependencies(${PROJECT_NAME}
    CoralReefPlayer
    SDL2::SDL2
    imgui
)
if(ENABLE_OPENCV)
    add_dependencies(${PROJECT_NAME}
        opencv_world
    )
endif(ENABLE_OPENCV)

install(TARGETS ${PROJECT_NAME}
    DESTINATION bin
)

install(FILES unifont-15.0.06.ttf
    DESTINATION bin
)

if(WIN32)
    install(IMPORTED_RUNTIME_ARTIFACTS SDL2::SDL2
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
    if(ENABLE_OPENCV)
        install(IMPORTED_RUNTIME_ARTIFACTS opencv_world
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin
        )
    endif(ENABLE_OPENCV)
endif(WIN32)
