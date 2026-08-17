# CHANGES.md

This document describes every change made to compile and run the Super Mario
World clone as a WebAssembly application in the browser, including Docker-based
deployment.

---

## Table of Contents

1. [Overview](#overview)
2. [File-by-file changes](#file-by-file-changes)
3. [How the WASM build works](#how-the-wasm-build-works)
4. [How the Emscripten main loop works](#how-the-emscripten-main-loop-works)
5. [How Docker serves the game](#how-docker-serves-the-game)
6. [How to build locally without Docker](#how-to-build-locally-without-docker)
7. [How to make changes](#how-to-make-changes)

---

## Overview

The project was originally a native C++23 application using SDL3, SDL3_image,
SDL3_ttf, GLM, tomlplusplus, and zstd. The goal was to compile the entire thing
to WebAssembly so it runs in a browser via Emscripten, with a Docker container
that builds and serves the result using nginx.

Three categories of changes were needed:

1. **Build system** -- CMake must detect Emscripten and produce a `.html` + `.js`
   + `.wasm` + `.data` bundle instead of a native executable.
2. **Source code** -- The main loop, file dialogs, and entry point all assume a
   native OS. They need `#ifdef __EMSCRIPTEN__` guards and an Emscripten-compatible
   callback-based loop.
3. **Deployment** -- nginx needs correct headers to serve WASM, and a Dockerfile
   orchestrates the build and serving.

---

## File-by-file changes

### CMakeLists.txt

**What changed and why:**

**1. Emscripten compiler flags (lines 12-16)**

```cmake
if(EMSCRIPTEN)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-template ..." CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-template ..." CACHE STRING "" FORCE)
endif()
```

- Forces static libraries because Emscripten cannot link shared libraries.
- Adds warning suppressions for vendored dependencies (SDL3, harfbuzz, freetype)
  that emit warnings Emscripten's Clang treats as errors.

**2. Harfbuzz pragma patch (lines 46-54)**

```cmake
if(EMSCRIPTEN AND TARGET harfbuzz)
    file(READ "${sdl3_ttf_SOURCE_DIR}/external/harfbuzz/src/hb.hh" _hb_hh_content)
    string(REPLACE
        "#pragma GCC diagnostic error   \"-Wunused\""
        "#pragma GCC diagnostic error   \"-Wunused\"\n#pragma GCC diagnostic ignored \"-Wunused-template\""
        _hb_hh_content "${_hb_hh_content}"
    )
    file(WRITE "${sdl3_ttf_SOURCE_DIR}/external/harfbuzz/src/hb.hh" "${_hb_hh_content}")
endif()
```

Harfbuzz's `hb.hh` header contains `#pragma GCC diagnostic error "-Wunused"`.
This pragma **overrides** command-line flags like `-Wno-unused-template` and
`-Wno-error`. The `-Wunused` category includes `-Wunused-template`, and unused
template operator() overloads in `hb-meta.hh` and `hb-algs.hh` cause ~20
compilation errors. The fix inserts `#pragma GCC diagnostic ignored
"-Wunused-template"` immediately after the error pragma, which takes precedence
for that specific sub-warning.

**3. Global Wno-error (line 102)**

```cmake
if(EMSCRIPTEN)
    add_compile_options(-Wno-error)
endif()
```

Demotes all warnings-as-errors to plain warnings for the project's own code.

**4. Emscripten linker options (lines 164-178)**

```cmake
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
```

| Flag | Purpose |
|------|---------|
| `--preload-file res@res` | Bundles `res/` into a `.data` file mounted at `res/` in the virtual filesystem |
| `-sFILESYSTEM=1` | Enables Emscripten's POSIX filesystem emulation |
| `-sALLOW_MEMORY_GROWTH=1` | Lets the WASM heap grow instead of crashing |
| `-sMAX_WEBGL_VERSION=2` | Enables WebGL2 for the SDL3 renderer |
| `-sFULL_ES3` | Full OpenGL ES 3 support |
| `-sWASM=1` | Emit WebAssembly (not asm.js) |
| `-sNO_DYNAMIC_EXECUTION` | Disables `eval()`/`new Function()` in JS glue (needed for COEP headers) |
| `SUFFIX ".html"` | Produces `super-mario-world.html` instead of an executable |

### src/defs/backend/engine.h

**What changed and why:**

- **Forward declarations** (lines 10-14): `Game`, `DeltaTime`, `TTF_Text` are now
  forward-declared because `LoopState` holds pointers to them.

- **`tick()` method** (line 46): Extracts one frame of the game loop into its own
  method so it can be called from both the native `while` loop and the Emscripten
  callback.

- **`LoopState` struct** (lines 69-76): Holds per-frame state that was previously
  local to `run()`: accumulated time, game/delta managers, FPS text, and an
  initialization flag. Must be a class member because the Emscripten callback
  needs access across multiple frames via a `void*` pointer.

- **`em_loop_callback`** (line 79): Static C-style callback function pointer for
  `emscripten_set_main_loop_arg`. Only exists when `__EMSCRIPTEN__` is defined.

### src/impl/backend/engine.cpp

**What changed and why:**

**1. Emscripten include (lines 13-15)**

```cpp
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
```

**2. Refactored `run()` (lines 81-116)**

The loop portion was extracted:

```cpp
// Native: blocking loop
while (m.running) {
    tick();
}

// Emscripten: callback-based, returns immediately
emscripten_set_main_loop_arg(em_loop_callback, this, 0, 1);
```

Emscripten cannot use a blocking loop because the browser's main thread must
return control to the event loop between frames. `emscripten_set_main_loop_arg`
registers a callback that Emscripten calls once per animation frame via
`requestAnimationFrame`. The third argument `0` means "use the browser's native
frame rate". The fourth argument `1` means "simulate infinite loop" (code after
this call is unreachable).

**3. `tick()` method (lines 118-149)**

Contains the single-frame logic: process inputs, clear render, update delta time,
update FPS display, tick game manager, draw text, present. On native, cleanup
happens here when `m.running` becomes false. On Emscripten, cleanup is deferred
to the callback.

**4. `em_loop_callback` (lines 151-167)**

```cpp
static auto em_loop_callback(void* arg) noexcept -> void {
    auto* engine = static_cast<backend::Engine*>(arg);
    if (!engine->m.running) {
        // cleanup ...
        emscripten_cancel_main_loop();
        return;
    }
    engine->tick();
}
```

Casts the `void*` back to `Engine*`, calls `tick()`, and handles cleanup when the
engine stops. Must explicitly call `emscripten_cancel_main_loop()` because the
Emscripten loop never truly "ends" -- it must be cancelled.

### src/main.cpp

**What changed and why:**

```cpp
#ifdef __EMSCRIPTEN__
#define SDL_MAIN_HANDLED
#endif
#include <SDL3/SDL_main.h>
```

SDL3's `SDL_main.h` redefines `main` to `SDL_main` via a preprocessor macro. On
Emscripten, the linker looks for a symbol called `main`. With the SDL macro
active, the compiled object exports `SDL_main` instead, causing:
```
error: undefined symbol: SDL_main
```
Defining `SDL_MAIN_HANDLED` before the include disables the macro.

### src/impl/sprites/editor.cpp

**What changed and why:**

The entire body of `sync_level()` is wrapped in `#ifndef __EMSCRIPTEN__`:

```cpp
auto sprite::Editor::sync_level(void) noexcept -> void {
#ifndef __EMSCRIPTEN__
    // SDL_ShowOpenFileDialog, SDL_ShowSaveFileDialog,
    // std::ifstream, std::ofstream, ZSTD_compress, ZSTD_decompress
#endif
}
```

This disables:
- `SDL_ShowOpenFileDialog` / `SDL_ShowSaveFileDialog` -- native file dialogs that
  don't exist in a browser.
- File I/O for level save/load which depends on those dialogs.
- ZSTD compression/decompression which is only used for level serialization.

The level editor still works (draw/erase tiles, switch brushes) but Ctrl+S/Ctrl+O
do nothing. To add web-based persistence later, use
`emscripten_run_script` for a JS file picker and read/write to the Emscripten
virtual filesystem.

### nginx.conf

**What changed and why:**

```nginx
server {
    listen 80;
    root /usr/share/nginx/html;
    index super-mario-world.html;

    location / {
        try_files $uri $uri/ /super-mario-world.html;
    }

    add_header Cross-Origin-Opener-Policy same-origin;
    add_header Cross-Origin-Embedder-Policy require-corp;
}
```

- **Removed custom `types {}` block**: It was replacing ALL of nginx's built-in
  MIME type mappings. Without `text/html` for `.html` files, the browser downloads
  the file instead of rendering it. nginx's default `mime.types` already handles
  `.html`, `.js`, and `.wasm`.

- **COOP/COEP headers**: Required for `SharedArrayBuffer` which Emscripten uses
  internally. Without these headers, the browser shows a security error.

### Dockerfile

**What changed and why:**

```dockerfile
FROM emscripten/emsdk:3.1.74 AS build
RUN apt-get update && apt-get install -y ninja-build wget && \
    wget .../cmake-3.31.6-linux-x86_64.tar.gz && \
    tar xzf ... -C /usr/local && ...
WORKDIR /src
COPY . .
RUN emcmake cmake -B build_wasm -G Ninja -DCMAKE_BUILD_TYPE=Release && \
    emmake ninja -C build_wasm -j$(nproc)

FROM nginx:stable-alpine
COPY nginx.conf /etc/nginx/conf.d/default.conf
COPY --from=build /src/build_wasm/bin/ /usr/share/nginx/html/
EXPOSE 80
```

Multi-stage build:
1. **Build stage**: Emscripten SDK + Ninja + CMake 3.31.6 (the emsdk image ships
   CMake 3.22 which is below our `cmake_minimum_required(VERSION 3.24)`).
   Compiles the entire project to WASM.
2. **Runtime stage**: nginx Alpine serving only the 4 WASM output files (~4MB).
   No build tools, no source code in the final image.

### .dockerignore

Excludes `build/`, `build_wasm/`, `.cache/`, `.git/`, `.vscode/` from the Docker
build context. Without this, the local `build_wasm/` directory (containing a
`CMakeCache.txt` with Windows paths) gets copied into the container, causing
CMake to fail.

### .gitignore

Added `build_wasm/` alongside the existing `build/` entry.

---

## How the WASM build works

1. `emcmake cmake` configures the project using Emscripten's CMake toolchain file
   (`Emscripten.cmake`), which sets the compiler to `em++`/`emcc` (wrappers around
   Clang targeting `wasm32-unknown-emscripten`).

2. `emmake ninja` compiles all source files to `.o` files using the Emscripten
   compiler, then links them with Emscripten's linker (`wasm-ld`). The linker
   produces:
   - `super-mario-world.html` -- the entry point you open in a browser
   - `super-mario-world.js` -- JavaScript glue code (event handling, WASM loading,
     filesystem emulation)
   - `super-mario-world.wasm` -- the compiled C++ code (~3.2MB)
   - `super-mario-world.data` -- preloaded assets from `res/` (~768KB)

3. The HTML file loads the JS glue, which loads the `.wasm` file, mounts the
   `.data` file into the virtual filesystem, and calls `main()`.

### Build commands (without Docker)

```sh
source /path/to/emsdk/emsdk_env.sh
emcmake cmake -B build_wasm -G Ninja -DCMAKE_BUILD_TYPE=Release
emmake ninja -C build_wasm
# Output in build_wasm/bin/
```

---

## How the Emscripten main loop works

Browsers are single-threaded and require the main thread to return control to the
event loop between frames. A `while(running) { tick(); }` loop would freeze the
browser tab.

Emscripten solves this with `emscripten_set_main_loop_arg`:

```
Native flow:
  main() -> create() -> run() -> [while loop: tick() tick() tick()...] -> destroy()

Emscripten flow:
  main() -> create() -> run() -> emscripten_set_main_loop_arg(callback) -> return
  ... later, browser calls callback once per frame:
  callback() -> tick()
  callback() -> tick()
  ... until emscripten_cancel_main_loop()
```

The `LoopState` struct on the `Engine` class holds all state that persists between
frames (game manager, delta time, FPS text). The static `em_loop_callback`
function receives a `void*` to the `Engine` instance, casts it back, and calls
`tick()`.

---

## How Docker serves the game

```
docker build -t super-mario-world .
docker run -p 8080:80 super-mario-world
```

1. Docker builds the WASM bundle inside the `emscripten/emsdk` container.
2. The build output (4 files) is copied to an nginx Alpine container.
3. nginx serves the files with correct MIME types and COOP/COEP headers.
4. Open `http://localhost:8080` in a browser.

The final image is ~25MB (nginx Alpine + WASM files). The build cache is NOT
included in the final image.

---

## How to build locally without Docker

**Requirements**: CMake >= 3.24, Ninja, Emscripten SDK installed and activated.

```sh
# Activate Emscripten
source /path/to/emsdk/emsdk_env.sh

# Configure (uses emcmake which sets the Emscripten CMake toolchain)
emcmake cmake -B build_wasm -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
emmake ninja -C build_wasm

# Output in build_wasm/bin/
# Serve with any HTTP server (WASM requires HTTP, not file://)
cd build_wasm/bin
python3 -m http.server 8080
# Open http://localhost:8080
```

---

## How to make changes

### Adding new source files

Add `.cpp` and `.h` files under `src/`. The project uses
`file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h")` so new
files are picked up automatically. Just re-run cmake (or let Ninja detect the
glob change) and rebuild.

### Adding new dependencies

Follow the existing `FetchContent` pattern in `CMakeLists.txt`:

```cmake
FetchContent_Declare(
    mylib
    GIT_REPOSITORY https://github.com/user/repo.git
    GIT_TAG        v1.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(mylib)
```

Then add it to `target_link_libraries`. If the dependency doesn't support
Emscripten, wrap it with `if(NOT EMSCRIPTEN)`.

### Modifying the game loop

The game loop lives in `engine.cpp`. Key points:

- **One frame = one call to `tick()`**. All per-frame logic should go in `tick()`.
- **State between frames** lives in the `LoopState` struct (`loop` member).
- **Cleanup** happens in two places: `tick()` for native (when `m.running` is
  false) and `em_loop_callback` for Emscripten. If you add new resources, clean
  up in both places.
- **Never add a blocking loop** inside `tick()` -- it runs once per frame.

### Adding platform-specific code

Use `#ifdef __EMSCRIPTEN__` / `#ifndef __EMSCRIPTEN__` for Emscripten-specific
code. The macro is defined automatically by the Emscripten compiler.

Common patterns:
- File dialogs: wrap in `#ifndef __EMSCRIPTEN__` (no native dialogs in browser)
- POSIX APIs: check Emscripten compatibility (most are emulated)
- Threading: Emscripten has limited pthreads support; prefer single-threaded
- `std::random_device`: may need a fallback seed in WASM

### Modifying the Docker/deployment setup

- **Nginx config**: edit `nginx.conf`. If adding new file types, nginx's default
  `mime.types` handles most common types. For exotic types, add a `location`
  block with `types { ... }`.
- **Dockerfile**: The build stage is slow (full compile). Docker layer caching
  helps -- source code changes only invalidate the `COPY . .` layer and rebuild.
  CMake/Ninja dependency downloads are cached.
- **Don't add build artifacts to git** -- `build_wasm/` is in `.gitignore`.

### Debugging WASM in the browser

1. Open browser DevTools (F12).
2. Check the Console tab for JS errors.
3. Check the Network tab to verify `.wasm` and `.data` files load correctly.
4. Emscripten's JS glue calls `console.log` for progress during loading.
5. Use `-sASSERTIONS=1` in linker flags for better error messages during
   development (add to `target_link_options` in CMakeLists.txt).
6. Use `-sDISABLE_EXCEPTION_CATCHING=0` to get C++ exception support for
   debugging (the project currently uses `-fno-exceptions`).
