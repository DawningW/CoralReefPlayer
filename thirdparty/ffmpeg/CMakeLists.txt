project(ffmpeg)

if(WIN32 OR CYGWIN)
    add_library(avdevice SHARED IMPORTED GLOBAL)
    set_target_properties(avdevice PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/avdevice-58.dll
        IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}avdevice${CMAKE_IMPORT_LIBRARY_SUFFIX}
    )
    add_library(avfilter SHARED IMPORTED GLOBAL)
    set_target_properties(avfilter PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/avfilter-7.dll
        IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}avfilter${CMAKE_IMPORT_LIBRARY_SUFFIX}
    )
    add_library(avformat SHARED IMPORTED GLOBAL)
    set_target_properties(avformat PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/avformat-58.dll
        IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}avformat${CMAKE_IMPORT_LIBRARY_SUFFIX}
    )
    add_library(avcodec SHARED IMPORTED GLOBAL)
    set_target_properties(avcodec PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/avcodec-58.dll
        IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}avcodec${CMAKE_IMPORT_LIBRARY_SUFFIX}
    )
    add_library(avutil SHARED IMPORTED GLOBAL)
    set_target_properties(avutil PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/avutil-56.dll
        IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}avutil${CMAKE_IMPORT_LIBRARY_SUFFIX}
    )
    add_library(postproc SHARED IMPORTED GLOBAL)
    set_target_properties(postproc PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/postproc-55.dll
        IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}postproc${CMAKE_IMPORT_LIBRARY_SUFFIX}
    )
    add_library(swresample SHARED IMPORTED GLOBAL)
    set_target_properties(swresample PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/swresample-3.dll
        IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}swresample${CMAKE_IMPORT_LIBRARY_SUFFIX}
    )
    add_library(swscale SHARED IMPORTED GLOBAL)
    set_target_properties(swscale PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/swscale-5.dll
        IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}swscale${CMAKE_IMPORT_LIBRARY_SUFFIX}
    )

    set_target_properties(avdevice avfilter avformat avcodec avutil postproc swresample swscale PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    install(IMPORTED_RUNTIME_ARTIFACTS avcodec avutil swresample swscale
        LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    )
elseif(ANDROID)
    add_library(avfilter SHARED IMPORTED GLOBAL)
    set_target_properties(avfilter PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Android/lib/${ANDROID_ABI}/libavfilter.so
    )
    add_library(avformat SHARED IMPORTED GLOBAL)
    set_target_properties(avformat PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Android/lib/${ANDROID_ABI}/libavformat.so
    )
    add_library(avcodec SHARED IMPORTED GLOBAL)
    set_target_properties(avcodec PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Android/lib/${ANDROID_ABI}/libavcodec.so
    )
    add_library(avutil SHARED IMPORTED GLOBAL)
    set_target_properties(avutil PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Android/lib/${ANDROID_ABI}/libavutil.so
    )
    add_library(swresample SHARED IMPORTED GLOBAL)
    set_target_properties(swresample PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Android/lib/${ANDROID_ABI}/libswresample.so
    )
    add_library(swscale SHARED IMPORTED GLOBAL)
    set_target_properties(swscale PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Android/lib/${ANDROID_ABI}/libswscale.so
    )

    set_target_properties(avfilter avformat avcodec avutil swresample swscale PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/Android/include
    )
elseif(IOS)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    if (NOT COREFOUNDATION_LIBRARY)
        message(STATUS "CoreFoundation.framework NOT found!")
    else()
        message(STATUS "CoreFoundation.framework found! ${COREFOUNDATION_LIBRARY}")
    endif()
    find_library(COREMEDIA_LIBRARY CoreMedia)
    if (NOT COREMEDIA_LIBRARY)
        message(STATUS "CoreMedia.framework NOT found!")
    else()
        message(STATUS "CoreMedia.framework found! ${COREMEDIA_LIBRARY}")
    endif()
    find_library(COREVIDEO_LIBRARY CoreVideo)
    if (NOT COREVIDEO_LIBRARY)
        message(STATUS "CoreVideo.framework NOT found!")
    else()
        message(STATUS "CoreVideo.framework found! ${COREVIDEO_LIBRARY}")
    endif()
    find_library(COREAUDIO_LIBRARY CoreAudio)
    if (NOT COREAUDIO_LIBRARY)
        message(STATUS "CoreAudio.framework NOT found!")
    else()
        message(STATUS "CoreAudio.framework found! ${COREAUDIO_LIBRARY}")
    endif()
    find_library(AVFOUNDATION_LIBRARY AVFoundation)
    if (NOT AVFOUNDATION_LIBRARY)
        message(STATUS "AVFoundation.framework NOT found!")
    else()
        message(STATUS "AVFoundation.framework found! ${AVFOUNDATION_LIBRARY}")
    endif()
    find_library(VIDEOTOOLBOX_LIBRARY VideoToolbox)
    if (NOT VIDEOTOOLBOX_LIBRARY)
        message(STATUS "VideoToolbox.framework NOT found!")
    else()
        message(STATUS "VideoToolbox.framework found! ${VIDEOTOOLBOX_LIBRARY}")
    endif()
    find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
    if (NOT AUDIOTOOLBOX_LIBRARY)
        message(STATUS "AudioToolbox.framework NOT found!")
    else()
        message(STATUS "AudioToolbox.framework found! ${AUDIOTOOLBOX_LIBRARY}")
    endif()

    add_library(avfilter STATIC IMPORTED GLOBAL)
    set_target_properties(avfilter PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/iOS/lib/libavfilter.a
    )
    add_library(avformat STATIC IMPORTED GLOBAL)
    set_target_properties(avformat PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/iOS/lib/libavformat.a
    )
    add_library(avcodec STATIC IMPORTED GLOBAL)
    set_target_properties(avcodec PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/iOS/lib/libavcodec.a
    )
    target_link_libraries(avcodec INTERFACE iconv z
        ${COREFOUNDATION_LIBRARY} ${COREMEDIA_LIBRARY} ${COREVIDEO_LIBRARY} ${COREAUDIO_LIBRARY}
        ${AVFOUNDATION_LIBRARY} ${VIDEOTOOLBOX_LIBRARY} ${AUDIOTOOLBOX_LIBRARY})
    add_library(avutil STATIC IMPORTED GLOBAL)
    set_target_properties(avutil PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/iOS/lib/libavutil.a
    )
    add_library(swresample STATIC IMPORTED GLOBAL)
    set_target_properties(swresample PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/iOS/lib/libswresample.a
    )
    add_library(swscale STATIC IMPORTED GLOBAL)
    set_target_properties(swscale PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/iOS/lib/libswscale.a
    )

    set_target_properties(avfilter avformat avcodec avutil swresample swscale PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/iOS/include
    )
else()
    add_library(avdevice SHARED IMPORTED GLOBAL)
    find_library(AVDEVICE_DIR avdevice)
    set_target_properties(avdevice PROPERTIES IMPORTED_LOCATION ${AVDEVICE_DIR})
    add_library(avfilter SHARED IMPORTED GLOBAL)
    find_library(AVFILTER_DIR avfilter)
    set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${AVFILTER_DIR})
    add_library(avformat SHARED IMPORTED GLOBAL)
    find_library(AVFORMAT_DIR avformat)
    set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${AVFORMAT_DIR})
    add_library(avcodec SHARED IMPORTED GLOBAL)
    find_library(AVCODEC_DIR avcodec)
    set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${AVCODEC_DIR})
    add_library(avutil SHARED IMPORTED GLOBAL)
    find_library(AVUTIL_DIR avutil)
    set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${AVUTIL_DIR})
    add_library(postproc SHARED IMPORTED GLOBAL)
    find_library(POSTPROC_DIR postproc)
    set_target_properties(postproc PROPERTIES IMPORTED_LOCATION ${POSTPROC_DIR})
    add_library(swresample SHARED IMPORTED GLOBAL)
    find_library(SWRESAMPLE_DIR swresample)
    set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${SWRESAMPLE_DIR})
    add_library(swscale SHARED IMPORTED GLOBAL)
    find_library(SWSCALE_DIR swscale)
    set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${SWSCALE_DIR})

    find_path(LIBAV_INCLUDE_DIR libavutil)
    set_target_properties(avdevice avfilter avformat avcodec avutil postproc swresample swscale PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${LIBAV_INCLUDE_DIR}
    )
endif()
