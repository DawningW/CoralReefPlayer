#!/bin/sh
cmake -B build -G Xcode -DCMAKE_TOOLCHAIN_FILE=../cmake/ios.toolchain.cmake -DPLATFORM=OS64COMBINED -DDEPLOYMENT_TARGET=14.0
cmake --build build --config Release
cmake --install build --config Release
