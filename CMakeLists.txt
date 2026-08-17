cmake_minimum_required(VERSION 3.24.0)
project(super-mario-world VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(EMSCRIPTEN)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-template -Wno-nontrivial-memcall -Wno-unused-but-set-variable -Wno-array-bounds -Wno-error" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-template -Wno-nontrivial-memcall -Wno-unused-but-set-variable -Wno-array-bounds -Wno-error" CACHE STRING "" FORCE)
endif()

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.4.2
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
    SDL3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG        release-3.4.0
    GIT_SHALLOW    TRUE
)

set(SDLIMAGE_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3_image)

FetchContent_Declare(
    SDL3_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG        release-3.2.2
)

set(SDLTTF_VENDORED ON)
FetchContent_MakeAvailable(SDL3_ttf)

if(EMSCRIPTEN AND TARGET harfbuzz)
    file(READ "${sdl3_ttf_SOURCE_DIR}/external/harfbuzz/src/hb.hh" _hb_hh_content)
    string(REPLACE
        "#pragma GCC diagnostic error   \"-Wunused\""
        "#pragma GCC diagnostic error   \"-Wunused\"\n#pragma GCC diagnostic ignored \"-Wunused-template\""
        _hb_hh_content "${_hb_hh_content}"
    )
    file(WRITE "${sdl3_ttf_SOURCE_DIR}/external/harfbuzz/src/hb.hh" "${_hb_hh_content}")
endif()

set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    telemetry
    GIT_REPOSITORY https://codeberg.org/resparing/telemetry.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(telemetry)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.3
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
    GIT_SHALLOW    TRUE
)

set(TOML_EXCEPTIONS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(tomlplusplus)

FetchContent_Declare(
    zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG        v1.5.6
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  build/cmake  # CMake is in subdirectory
)


set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(zstd)

if(EMSCRIPTEN)
    add_compile_options(-Wno-error)
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h")

if(APPLE)
    set(APP_ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/res/images/logo.png)

    set_source_files_properties(${APP_ICON_FILE} PROPERTIES 
        MACOSX_PACKAGE_LOCATION "Resources"
    )

    add_executable(${PROJECT_NAME} MACOSX_BUNDLE
        ${SOURCES}
        ${APP_ICON_FILE}
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "@executable_path"
        MACOSX_BUNDLE TRUE
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_ICON_FILE "logo.png"
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "Super Mario World"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.untitled.${PROJECT_NAME}"
    )
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

if(WIN32 AND NOT MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++ -static)

    target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_MAIN_HANDLED)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-exceptions)
elseif(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /EHs-c)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _HAS_EXCEPTIONS=0)
else()
    message(FATAL_ERROR "Unsupported compiler for no-exceptions build")
endif()


target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE
    telemetry
    SDL3::SDL3-static
    SDL3_image::SDL3_image-static
    SDL3_ttf::SDL3_ttf-static
    glm::glm
    tomlplusplus::tomlplusplus
    libzstd_static
)

if(EMSCRIPTEN)
    target_link_options(${PROJECT_NAME} PRIVATE
        "--preload-file" "${CMAKE_SOURCE_DIR}/res@res"
        "-sFILESYSTEM=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sMAX_WEBGL_VERSION=2"
        "-sFULL_ES3"
        "-sWASM=1"
        "-sNO_DYNAMIC_EXECUTION"
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        SUFFIX ".html"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
