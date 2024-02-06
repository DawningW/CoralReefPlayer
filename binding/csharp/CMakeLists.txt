project(coralreefplayercs VERSION ${CoralReefPlayer_VERSION})

include(copy_libs)

find_program(DOTNET_EXECUTABLE NAMES dotnet)
if(NOT DOTNET_EXECUTABLE)
    message(FATAL_ERROR "C# binding needs dotnet cli to build!")
    return()
endif()

file(
    COPY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

# Runtime IDentifier
# see: https://docs.microsoft.com/en-us/dotnet/core/rid-catalog
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)")
    set(DOTNET_PLATFORM arm64)
else()
    set(DOTNET_PLATFORM x64)
endif()

if(WIN32)
    set(DOTNET_RID win-${DOTNET_PLATFORM})
elseif(APPLE)
    set(DOTNET_RID osx-${DOTNET_PLATFORM})
else()
    set(DOTNET_RID linux-${DOTNET_PLATFORM})
endif()
message(STATUS ".Net RID: ${DOTNET_RID}")

# Language Version
# see: https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/configure-language-version
set(DOTNET_LANG "8.0" CACHE STRING "Specify the C# language version (default \"8.0\")")
message(STATUS ".Net C# language version: ${DOTNET_LANG}")

# Target Framework
# see: https://docs.microsoft.com/en-us/dotnet/standard/frameworks
# see: https://learn.microsoft.com/en-us/dotnet/standard/net-standard
option(USE_DOTNET_35 "Use .Net Framework 3.5" OFF)
option(USE_DOTNET_47 "Use .Net Framework 4.7" OFF)
option(USE_DOTNET_471 "Use .Net Framework 4.7.1" OFF)
option(USE_DOTNET_472 "Use .Net Framework 4.7.2" OFF)
option(USE_DOTNET_48 "Use .Net Framework 4.8" OFF)
option(USE_DOTNET_481 "Use .Net Framework 4.8.1" OFF)
option(USE_DOTNET_STD_20 "Use .Net Standard 2.0" ON)
option(USE_DOTNET_STD_21 "Use .Net Standard 2.1" OFF)
# .Net Core 3.1 LTS is not available for osx arm64
if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)")
    set(USE_DOTNET_CORE_31 OFF)
else()
    option(USE_DOTNET_CORE_31 "Use .Net Core 3.1 LTS" OFF)
endif()
option(USE_DOTNET_6 "Use .Net 6 LTS" ON)
option(USE_DOTNET_7 "Use .Net 7" OFF)
option(USE_DOTNET_8 "Use .Net 8 LTS" OFF)

if(USE_DOTNET_35)
    list(APPEND TFM_LIST "net35")
endif()
if(USE_DOTNET_47)
    list(APPEND TFM_LIST "net47")
endif()
if(USE_DOTNET_471)
    list(APPEND TFM_LIST "net471")
endif()
if(USE_DOTNET_472)
    list(APPEND TFM_LIST "net472")
endif()
if(USE_DOTNET_48)
    list(APPEND TFM_LIST "net48")
endif()
if(USE_DOTNET_481)
    list(APPEND TFM_LIST "net481")
endif()
if(USE_DOTNET_STD_20)
    list(APPEND TFM_LIST "netstandard2.0")
endif()
if(USE_DOTNET_STD_21)
    list(APPEND TFM_LIST "netstandard2.1")
endif()
if(USE_DOTNET_CORE_31)
    list(APPEND TFM_LIST "netcoreapp3.1")
endif()
if(USE_DOTNET_6)
    list(APPEND TFM_LIST "net6.0")
endif()
if(USE_DOTNET_7)
    list(APPEND TFM_LIST "net7.0")
endif()
if(USE_DOTNET_8)
    list(APPEND TFM_LIST "net8.0")
endif()

list(LENGTH TFM_LIST TFM_LIST_LENGTH)
if(TFM_LIST_LENGTH EQUAL "0")
    message(FATAL_ERROR "No .Net SDK selected!")
endif()
string(JOIN ";" DOTNET_TFM ${TFM_LIST})
message(STATUS ".Net TargetFrameworks: ${DOTNET_TFM}")
if(TFM_LIST_LENGTH GREATER "1")
    string(CONCAT DOTNET_TFM "<TargetFrameworks>" "${DOTNET_TFM}" "</TargetFrameworks>")
else()
    string(CONCAT DOTNET_TFM "<TargetFramework>" "${DOTNET_TFM}" "</TargetFramework>")
endif()

# Project config
set(DOTNET_PROJECT "CoralReefPlayer")
message(STATUS ".Net project: ${DOTNET_PROJECT}")
set(DOTNET_PROJECT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${DOTNET_PROJECT})
message(STATUS ".Net project build path: ${DOTNET_PROJECT_DIR}")

set(DOTNET_NATIVE_PROJECT ${DOTNET_PROJECT}.Runtime.${DOTNET_RID})
message(STATUS ".Net runtime project: ${DOTNET_NATIVE_PROJECT}")
set(DOTNET_NATIVE_PROJECT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${DOTNET_PROJECT}.Runtime)
message(STATUS ".Net runtime project build path: ${DOTNET_NATIVE_PROJECT_DIR}")

configure_file(
    ${CMAKE_CURRENT_BINARY_DIR}/Directory.Build.props.in
    ${CMAKE_CURRENT_BINARY_DIR}/Directory.Build.props
)

configure_file(
    ${DOTNET_PROJECT_DIR}/${DOTNET_PROJECT}.csproj.in
    ${DOTNET_PROJECT_DIR}/${DOTNET_PROJECT}.csproj
)

configure_file(
    ${DOTNET_NATIVE_PROJECT_DIR}/${DOTNET_PROJECT}.Runtime.csproj.in
    ${DOTNET_NATIVE_PROJECT_DIR}/${DOTNET_NATIVE_PROJECT}.csproj
)

configure_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${DOTNET_PROJECT}.Test/${DOTNET_PROJECT}.Test.csproj.in
    ${CMAKE_CURRENT_BINARY_DIR}/${DOTNET_PROJECT}.Test/${DOTNET_PROJECT}.Test.csproj
)

add_custom_target(csharp-prepare
    COMMAND echo "Build C# binding"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS CoralReefPlayer
)
add_copy_libs_command(csharp-prepare ${DOTNET_PROJECT}.Runtime/)

add_custom_target(csharp-package
    COMMAND ${DOTNET_EXECUTABLE} build -c Release ${DOTNET_NATIVE_PROJECT_DIR}
    COMMAND ${DOTNET_EXECUTABLE} pack -c Release ${DOTNET_NATIVE_PROJECT_DIR}
    COMMAND ${DOTNET_EXECUTABLE} build -c Release ${DOTNET_PROJECT_DIR}
    COMMAND ${DOTNET_EXECUTABLE} pack -c Release ${DOTNET_PROJECT_DIR}
    DEPENDS csharp-prepare
)
