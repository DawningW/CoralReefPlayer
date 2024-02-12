project(coralreefplayer4j VERSION ${CoralReefPlayer_VERSION})

if(NOT ANDROID)
    include(copy_libs)

    find_package(Java 1.8 COMPONENTS Runtime Development)
    if(NOT Java_FOUND)
        message(FATAL_ERROR "Java binding needs Java >= 8 to build!")
        return()
    endif()
    find_package(JNI REQUIRED)

    file(
        COPY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    )

    configure_file(
        ${CMAKE_CURRENT_BINARY_DIR}/gradle.properties.in
        ${CMAKE_CURRENT_BINARY_DIR}/gradle.properties
        @ONLY
    )
else()
    set(JNI_INCLUDE_DIRS "")
    set(JNI_LIBRARIES "android")
endif()

add_library(crp_native SHARED
    src/main/jni/cn_oureda_coralreefplayer_NativeMethods.cpp
)

target_include_directories(crp_native
    PRIVATE
        ${JNI_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main/jni
)

target_compile_definitions(crp_native
    PRIVATE
        $<$<BOOL:${ANDROID}>:ANDROID>
)

target_link_libraries(crp_native
    PRIVATE
        ${JNI_LIBRARIES}
        CoralReefPlayer
)

add_dependencies(crp_native
    CoralReefPlayer
)

if(NOT ANDROID)
    add_custom_target(java-prepare
        COMMAND echo "Build java binding"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS crp_native
    )
    add_copy_libs_command(java-prepare src/main/resources/native/)
    add_custom_command(
        TARGET java-prepare POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:crp_native> src/main/resources/native/
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_custom_target(java-package
        COMMAND gradlew publish
        DEPENDS java-prepare
    )
endif()
