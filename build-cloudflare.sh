#!/usr/bin/env bash
set -euo pipefail

# Cloudflare Workers Builds: compile WASM with Emscripten, output to ./public.
#
# The full toolchain (cmake, ninja, ccache, emsdk) lives in
# node_modules/.toolchain so Cloudflare's dependency cache -- keyed on
# package-lock.json -- restores it on every rebuild. Result: after the first
# build (~7 min), subsequent builds skip all downloads and reuse ccache hits
# (~1-2 min when only game sources change).
#
# Dashboard settings (Workers Builds):
#   Build command:   npm run build
#   Deploy command:  npx wrangler deploy
#   Production branch: workers   (rebuilds only when you push to `workers`)

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PUBLIC_DIR="$ROOT/public"
BUILD_DIR="$ROOT/build_wasm"
TOOLCHAIN="$ROOT/node_modules/.toolchain"
EMSDK_DIR="$TOOLCHAIN/emsdk"
EMSDK_VERSION="3.1.74"
CMAKE_VERSION="3.31.6"
NINJA_VERSION="1.12.1"
CCACHE_VERSION="4.10.2"

for t in git python3; do
  if ! command -v "$t" >/dev/null 2>&1; then
    echo "Missing required tool: $t" >&2
    exit 1
  fi
done

mkdir -p "$TOOLCHAIN"

download() { # download <url> <dest>: curl, wget, or python fallback
  if command -v curl >/dev/null 2>&1; then
    curl -fsSL -o "$2" "$1"
  elif command -v wget >/dev/null 2>&1; then
    wget -O "$2" "$1"
  else
    python3 -c "import urllib.request,sys; urllib.request.urlretrieve(sys.argv[1], sys.argv[2])" "$1" "$2"
  fi
}

if [ ! -x "$TOOLCHAIN/cmake/bin/cmake" ]; then
  echo "==> Installing CMake $CMAKE_VERSION (cached in node_modules/.toolchain)..."
  TGZ="$TOOLCHAIN/cmake.tar.gz"
  [ -f "$TGZ" ] || download \
    "https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION-linux-x86_64.tar.gz" "$TGZ"
  mkdir -p "$TOOLCHAIN/cmake"
  tar xzf "$TGZ" --strip-components=1 -C "$TOOLCHAIN/cmake"
fi

if [ ! -x "$TOOLCHAIN/ninja/ninja" ]; then
  echo "==> Installing Ninja $NINJA_VERSION (cached in node_modules/.toolchain)..."
  ZIP="$TOOLCHAIN/ninja.zip"
  [ -f "$ZIP" ] || download \
    "https://github.com/ninja-build/ninja/releases/download/v$NINJA_VERSION/ninja-linux.zip" "$ZIP"
  mkdir -p "$TOOLCHAIN/ninja"
  python3 -c "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])" "$ZIP" "$TOOLCHAIN/ninja"
  chmod +x "$TOOLCHAIN/ninja/ninja"
fi

if [ ! -x "$TOOLCHAIN/ccache/ccache" ]; then
  echo "==> Installing ccache $CCACHE_VERSION (cached in node_modules/.toolchain)..."
  XZ="$TOOLCHAIN/ccache.tar.xz"
  [ -f "$XZ" ] || download \
    "https://github.com/ccache/ccache/releases/download/v$CCACHE_VERSION/ccache-$CCACHE_VERSION-linux-x86_64.tar.xz" "$XZ"
  mkdir -p "$TOOLCHAIN/ccache"
  tar xJf "$XZ" --strip-components=1 -C "$TOOLCHAIN/ccache"
fi

if [ ! -d "$EMSDK_DIR" ]; then
  echo "==> Setting up Emscripten $EMSDK_VERSION (cached in node_modules/.toolchain)..."
  git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
fi

cd "$EMSDK_DIR"
./emsdk install "$EMSDK_VERSION"
./emsdk activate "$EMSDK_VERSION"
# shellcheck disable=SC1091
source ./emsdk_env.sh
cd "$ROOT"

export PATH="$TOOLCHAIN/cmake/bin:$TOOLCHAIN/ninja:$TOOLCHAIN/ccache:$PATH"
export CCACHE_DIR="$TOOLCHAIN/ccache-data"
export CCACHE_MAXSIZE=3G

echo "cmake: $(command -v cmake) ($(cmake --version | head -n1))"
echo "ninja: $(command -v ninja) ($(ninja --version))"
echo "ccache: $(command -v ccache) ($(ccache --version | head -n1))"

echo "==> Building WASM..."
emcmake cmake -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
emmake ninja -C "$BUILD_DIR" -j"$(nproc 2>/dev/null || echo 4)"

echo "ccache stats:"
ccache --show-stats | head -n 12 || true

echo "==> Publishing to $PUBLIC_DIR..."
mkdir -p "$PUBLIC_DIR"
# Copy build output (html/js/wasm/data) into public/
cp -f "$BUILD_DIR"/bin/* "$PUBLIC_DIR"/

# Cloudflare serves / -> index.html, nginx served super-mario-world.html.
# Keep both names so old links keep working. Always refresh index.html
# (overwrites the placeholder below once the real build exists).
if [ -f "$PUBLIC_DIR/super-mario-world.html" ]; then
  cp -f "$PUBLIC_DIR/super-mario-world.html" "$PUBLIC_DIR/index.html"
fi

# Required headers (replaces nginx.conf COOP/COEP). Content-Encoding br
# below matches what Cloudflare emits for wasm/js/data, keep consistent.
cat > "$PUBLIC_DIR/_headers" <<'EOF'
/*
  Cross-Origin-Opener-Policy: same-origin
  Cross-Origin-Embedder-Policy: require-corp
/super-mario-world.wasm
  Content-Type: application/wasm
  Cache-Control: public, max-age=31536000, immutable
/super-mario-world.data
  Cache-Control: public, max-age=31536000, immutable
/super-mario-world.js
  Cache-Control: public, max-age=31536000, immutable
EOF

echo "==> public/ contents:"
ls -lh "$PUBLIC_DIR"