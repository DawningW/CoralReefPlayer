project(imgui)

# Must set c++11 on MacOS
set(CMAKE_CXX_STANDARD 11)

add_library(${PROJECT_NAME} STATIC
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    backends/imgui_impl_sdl2.cpp
    backends/imgui_impl_sdlrenderer.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL2::SDL2
)

add_dependencies(${PROJECT_NAME}
    SDL2::SDL2
)

# install(TARGETS ${PROJECT_NAME}
#     LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
#     ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
# )
