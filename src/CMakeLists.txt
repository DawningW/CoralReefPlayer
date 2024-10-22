set(CMAKE_CXX_STANDARD 20)

if(BUILD_SHARED_LIBS)
    set(CRP_SHARED_LIB 1)
else()
    set(CRP_SHARED_LIB 0)
endif()

configure_file(
    version.h.in
    generated/version.h
    @ONLY
)

configure_file(
    coralreefplayer.h.in
    generated/coralreefplayer.h
    @ONLY
)

add_custom_target(git_hash
    COMMAND
        ${CMAKE_COMMAND} -D TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} -P ${CMAKE_MODULE_DIR}/git-hash.cmake
    WORKING_DIRECTORY
        ${CoralReefPlayer_SOURCE_DIR}
)

add_library(${PROJECT_NAME}
    ${CMAKE_CURRENT_BINARY_DIR}/generated/coralreefplayer.h
    dllmain.cpp
    AsyncCallback.hpp
    Matcher.hpp
    StreamPuller.cpp
    StreamSink.cpp
    VideoDecoder.cpp
    AudioDecoder.cpp
    $<$<STREQUAL:$<TARGET_PROPERTY:${PROJECT_NAME},TYPE>,SHARED_LIBRARY>:
        $<$<OR:$<BOOL:${WIN32}>,$<BOOL:${CYGWIN}>>:CoralReefPlayer.rc>>
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/.
        ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        CORALREEFPLAYER_EXPORTS
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        live555
        httplib
        avcodec
        avutil
        swscale
        swresample
)

add_dependencies(${PROJECT_NAME}
    git_hash
    live555
    httplib
    avcodec
    avutil
    swscale
    swresample
)

if(APPLE AND BUILD_MACOS_FRAMEWORK)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        MACOSX_FRAMEWORK_BUNDLE_VERSION ${PROJECT_VERSION}.0.0
        MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${PROJECT_VERSION}.0
        VERSION ${PROJECT_VERSION}.0 # current version
        SOVERSION 1.0.0 # compatibility version
        PUBLIC_HEADER coralreefplayer.h
        # XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
    )

    install(TARGETS ${PROJECT_NAME}
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    )

    if(BUILD_DEMO AND BUILD_MACOS_BUNDLE)
        install(TARGETS ${PROJECT_NAME}
            FRAMEWORK DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}/CoralReefCam.app/Contents/Frameworks
        )
    endif()
else()
    install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    )

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/coralreefplayer.h
        DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}
    )
endif()
