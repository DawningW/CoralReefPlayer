project(implot)

# Same as imgui
set(CMAKE_CXX_STANDARD 11)

add_library(${PROJECT_NAME} STATIC
    implot.cpp
    implot_demo.cpp
    implot_items.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        imgui
)

add_dependencies(${PROJECT_NAME}
    imgui
)
