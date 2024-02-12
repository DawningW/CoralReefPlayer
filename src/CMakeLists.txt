set(CMAKE_CXX_STANDARD 20)

configure_file(
    version.h.in
    generated/version.h
    @ONLY
)

add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/generated/git_hash.h
        ALL
    COMMAND
        ${CMAKE_COMMAND} -D TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} -P ${CMAKE_MODULE_DIR}/git-hash.cmake
    WORKING_DIRECTORY
        ${CoralReefPlayer_SOURCE_DIR}
)

add_custom_target(git_hash ALL
    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/generated/git_hash.h
)

add_library(${PROJECT_NAME} SHARED
    dllmain.cpp
    AsyncCallback.hpp
    Matcher.hpp
    StreamPuller.cpp
    StreamSink.cpp
    VideoDecoder.cpp
    $<$<OR:$<BOOL:${WIN32}>,$<BOOL:${CYGWIN}>>:CoralReefPlayer.rc>
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/.
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        CORALREEFPLAYER_EXPORTS
        ENABLE_MJPEG_OVER_HTTP=$<BOOL:${ENABLE_MJPEG_OVER_HTTP}>
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${CURL_LIBS}
        live555
        avcodec
        avutil
        swscale
        swresample
)

add_dependencies(${PROJECT_NAME}
    git_hash
    ${CURL_LIBS}
    live555
    avcodec
    avutil
    swscale
    swresample
)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
)

install(FILES coralreefplayer.h 
    DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}
)
