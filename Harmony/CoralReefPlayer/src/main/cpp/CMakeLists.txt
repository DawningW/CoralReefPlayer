# the minimum version of CMake.
cmake_minimum_required(VERSION 3.5.0)
project(coralreefplayer-ohos)

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

set(NODE_ADDON_API_OHOS_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../oh_modules/@ohos-rs/node-addon-api)
set(CMAKE_MODULE_PATH ${NODE_ADDON_API_OHOS_ROOT_PATH})

find_package(node_addon_api_ohos REQUIRED)

add_subdirectory(../../../../.. CoralReefPlayer)
