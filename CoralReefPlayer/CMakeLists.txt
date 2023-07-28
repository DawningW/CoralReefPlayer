project(CoralReefPlayer)

set(CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME} SHARED
    dllmain.cpp
    StreamPuller.cpp
    StreamSink.cpp
    $<$<OR:$<BOOL:${WIN32}>,$<BOOL:${CYGWIN}>>:CoralReefPlayer.rc>
)

target_include_directories(${PROJECT_NAME}
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/.
)

target_compile_definitions(${PROJECT_NAME}
PRIVATE
    CORALREEFPLAYER_EXPORTS
)

target_link_libraries(${PROJECT_NAME}
PRIVATE
    live555
    avformat
    avcodec
    avutil
    swscale
)

add_dependencies(${PROJECT_NAME}
    live555
    avformat
    avcodec
    avutil
    swscale
)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES coralreefplayer.h 
    DESTINATION include
)
