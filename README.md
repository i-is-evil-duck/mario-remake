# Super Mario World

<br />

<img alt="Stargazers" src="https://img.shields.io/badge/dynamic/json?url=https://codeberg.org/api/v1/repos/resparing/mario&query=$.stars_count&label=stars&style=for-the-badge&logo=starship&color=C9CBFF&logoColor=D9E0EE&labelColor=302D41">

## Views

<img src="https://count.getloli.com/@mario-cpp?name=mario-cpp&theme=booru-lewd&padding=7&offset=0&align=top&scale=1&pixelated=1&darkmode=auto" />

## Building

### Requirements

- CMake >= 3.24
- Ninja
- C++23 compiler (GCC 13+, Clang 16+, or MSVC 17+)
- For WASM: [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)

### Native build (Linux/macOS)

```sh
./run.sh
```

### Native build (Windows)

```powershell
.\run.bat
```

### WebAssembly build

```sh
source /path/to/emsdk/emsdk_env.sh
emcmake cmake -B build_wasm -G Ninja -DCMAKE_BUILD_TYPE=Release
emmake ninja -C build_wasm
```

Output will be in `build_wasm/bin/`.

## Docker (WASM)

Build and serve the game in a container:

```sh
docker build -t super-mario-world .
docker run -p 8080:80 super-mario-world
```

Then open http://localhost:8080 in your browser.

## Key binds

| Key | Action |
|-----|--------|
| `A` / `Left Arrow` | Move left |
| `D` / `Right Arrow` | Move right |
| `W` / `Up Arrow` | Jump |
| `Escape` | Toggle editing mode |

### Editor

| Key | Action |
|-----|--------|
| `1` | Draw unordered/important tiles |
| `2` | Draw pipe tiles |
| `3` | Draw semisolid tiles |
| `4` | Draw mushroom tiles |
| `5` | Draw decoration tiles |
| `6` | Draw ground tiles |
| `E` | Select tile under mouse |
| `Ctrl+S` | Save level |
| `Ctrl+O` | Open level |
| Left click | Draw |
| Right click | Destroy |
