project(implot)

add_library(${PROJECT_NAME} STATIC
    implot.cpp
    implot_demo.cpp
    implot_items.cpp
)

target_include_directories(${PROJECT_NAME}
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/.
)

target_link_libraries(${PROJECT_NAME}
PUBLIC
    imgui
)

add_dependencies(${PROJECT_NAME}
    imgui
)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
