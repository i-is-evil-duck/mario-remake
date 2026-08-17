#!/usr/bin/env bash
set -euo pipefail

BUILD_DIRECTORY=build
MODE=Debug

if [ ! -d ${BUILD_DIRECTORY} ]; then
    mkdir ${BUILD_DIRECTORY}

    cmake -B ${BUILD_DIRECTORY} -G "Ninja Multi-Config"
fi

cmake --build ${BUILD_DIRECTORY} --config ${MODE} -j

# open build/${MODE}/super-mario-world.app
./${BUILD_DIRECTORY}/${MODE}/super-mario-world.app/Contents/MacOS/super-mario-world
