set(CMAKE_CXX_STANDARD 23)

include(copy_libs)

enable_testing()

add_executable(pull_test pull_test.cpp)
if(EMSCRIPTEN)
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<BOOL:${USE_PTHREAD}>:-sPTHREAD_POOL_SIZE=3>
        -sASYNCIFY=1
    )
endif()
target_link_libraries(pull_test PRIVATE ${PROJECT_NAME})
if(MINGW)
    target_link_libraries(pull_test PRIVATE stdc++exp)
endif()
add_dependencies(pull_test ${PROJECT_NAME})
add_copy_libs_command(pull_test .)

add_test(
    NAME pull_test
    COMMAND $<TARGET_FILE:pull_test>
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
