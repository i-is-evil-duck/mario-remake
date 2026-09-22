# Super Mario World

## Play live: https://mario.j3ly.com

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

## Cloudflare (live at https://mario.j3ly.com)

The game is deployed with **Cloudflare Workers Static Assets** (see
`wrangler.toml`, `package.json`, `build-cloudflare.sh`).

Deployment flow:

- **`workers` branch** — Cloudflare Workers Builds watches only this branch.
  Push to it whenever you want to publish, so everyday commits to `master`
  don't trigger 7-minute rebuilds:
  ```sh
  git checkout workers && git merge master && git push origin workers
  ```
- **Build toolchain is cached**: cmake/ninja/ccache/emsdk live in
  `node_modules/.toolchain`, which Cloudflare's dependency cache (keyed on
  `package-lock.json`) restores between builds. First build takes ~7 min;
  later rebuilds reuse ccache and only recompile changed game sources.

Workers Builds dashboard settings:

| Setting | Value |
|---------|-------|
| Build command | `npm run build` |
| Deploy command | `npx wrangler deploy` |
| Production branch | `workers` |

Custom domain `mario.j3ly.com` is a CNAME to the Worker.

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

## Credits

This is a WebAssembly port of [Super Mario World](https://codeberg.org/resparing/mario) by [resparing](https://codeberg.org/resparing).

The original native C++23 + SDL3 project, level editor, and all game assets (sprites, tiles, fonts, level data) were created by resparing. This fork adds Emscripten/WASM compilation support and Docker-based deployment.

## Zig prototype controls (merged from github master)

Reset key (R) - restarts the game

Map Builder (B) - toggles builder mode with controls:

Left click - Place tile
Right click - Erase tile
1-9 - Select tile type (Ground, Brick, Question, Pipe, PipeTop, Invisible, Platform, Stair, Pole, Flag)
G - Place Goomba
M - Place Mushroom
F - Place Fire Flower
S - Save level to JSON
P/O - Scroll camera (next/previous chunk)
B - Exit builder
