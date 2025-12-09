project(coralreefplayer-wasm VERSION ${CoralReefPlayer_VERSION})

set(CMAKE_CXX_STANDARD 20)

include(copy_libs)

find_program(NodeJS_EXECUTABLE NAMES node)
if(NOT NodeJS_EXECUTABLE)
    message(FATAL_ERROR "WebAssembly binding needs Node.js >= 12 to build!")
    return()
endif()

set(USE_MODULE ON CACHE BOOL "Export UMD or ES6 module for wasm binding")
set(USE_ES6_MODULE OFF CACHE BOOL "Export ES6 module for wasm binding")

if(USE_PTHREAD)
    set(MT_SUFFIX "-mt")
endif()

file(
    COPY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    PATTERN "main.cpp" EXCLUDE
)

configure_file(
    ${CMAKE_CURRENT_BINARY_DIR}/package.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/package.json
    @ONLY
)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_options(${PROJECT_NAME}
    PRIVATE
        $<$<BOOL:${USE_PTHREAD}>:-sPTHREAD_POOL_SIZE=3>
        $<$<NOT:$<BOOL:${USE_PTHREAD}>>:-sASYNCIFY=1>
        -sEXPORTED_FUNCTIONS=["_malloc","_free"]
        # -sPURE_WASI=1 # Not support running in browser yet if enable this
        $<$<BOOL:${USE_MODULE}>:-sMODULARIZE=1>
        $<$<BOOL:${USE_ES6_MODULE}>:-sEXPORT_ES6=1>
        --no-entry
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        embind
        $<LINK_LIBRARY:WHOLE_ARCHIVE,CoralReefPlayer>
)

add_dependencies(${PROJECT_NAME}
    CoralReefPlayer
)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "coralreefplayer")

add_custom_target(wasm-package
    COMMAND npm run package -- --pack-destination ${CMAKE_INSTALL_PREFIX}
    DEPENDS ${PROJECT_NAME}
)
