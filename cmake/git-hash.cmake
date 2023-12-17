# in case Git is not available, we default to "unknown"
set(GIT_HASH "unknown")

# find Git and if available set GIT_HASH variable
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --abbrev=7 --dirty --always
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

message(STATUS "Git hash is ${GIT_HASH}")

# generate file git_hash.h
file(WRITE
    ${TARGET_DIR}/generated/git_hash.h
    "#define CRP_GIT_HASH \"${GIT_HASH}\"\n" # MSVC .rc need LF
)
