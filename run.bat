@echo off
setlocal enabledelayedexpansion

set BUILD_DIRECTORY=build
set MODE=Debug

if not exist %BUILD_DIRECTORY% (
    mkdir %BUILD_DIRECTORY%

    cmake -B %BUILD_DIRECTORY% -G "Ninja Multi-Config"
)

cmake --build %BUILD_DIRECTORY% --config %MODE% -j

.\%BUILD_DIRECTORY%\%MODE%\super-mario-world.exe
