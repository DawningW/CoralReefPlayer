cmake_minimum_required(VERSION 3.14)

project(CoralReefPlayerIOS)

set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/CoralReefPlayer/Library CACHE PATH "" FORCE)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(BUILD_DEMO OFF CACHE BOOL "" FORCE)

add_subdirectory(.. CoralReefPlayer)

# cmake -B build -G Xcode -DCMAKE_TOOLCHAIN_FILE=../cmake/ios.toolchain.cmake -DPLATFORM=OS64COMBINED -DDEPLOYMENT_TARGET=14.0
# cmake --build build --config Release
# cmake --install build --config Release
