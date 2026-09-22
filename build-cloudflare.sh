#!/usr/bin/env bash
set -euo pipefail

# Cloudflare Workers Builds: compile WASM with Emscripten, output to ./public.
# Dashboard settings:
#   Build command:   npm run build
#   Deploy command:  npx wrangler deploy

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PUBLIC_DIR="$ROOT/public"
BUILD_DIR="$ROOT/build_wasm"
CACHE_DIR="$ROOT/.cache"
EMSDK_DIR="${EMSDK_DIR:-$CACHE_DIR/emsdk}"
EMSDK_VERSION="3.1.74"

# No root in Cloudflare Workers Builds, so fetch user-space binaries.
CMAKE_VERSION="3.31.6"
NINJA_VERSION="1.12.1"

echo "==> Checking tools..."
for t in git python3; do
  if ! command -v "$t" >/dev/null 2>&1; then
    echo "Missing required tool: $t" >&2
    exit 1
  fi
done

mkdir -p "$CACHE_DIR"

download() { # download <url> <dest>: curl, wget, or python fallback
  if command -v curl >/dev/null 2>&1; then
    curl -fsSL -o "$2" "$1"
  elif command -v wget >/dev/null 2>&1; then
    wget -O "$2" "$1"
  else
    python3 -c "import urllib.request,sys; urllib.request.urlretrieve(sys.argv[1], sys.argv[2])" "$1" "$2"
  fi
}

if ! command -v cmake >/dev/null 2>&1; then
  echo "==> Installing CMake $CMAKE_VERSION (user-space, no root)..."
  CMAKE_TGZ="$CACHE_DIR/cmake-$CMAKE_VERSION-linux-x86_64.tar.gz"
  if [ ! -f "$CMAKE_TGZ" ]; then
    download \
      "https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION-linux-x86_64.tar.gz" \
      "$CMAKE_TGZ"
  fi
  mkdir -p "$CACHE_DIR/cmake"
  tar xzf "$CMAKE_TGZ" --strip-components=1 -C "$CACHE_DIR/cmake"
fi

if ! command -v ninja >/dev/null 2>&1; then
  echo "==> Installing Ninja $NINJA_VERSION (user-space, no root)..."
  NINJA_ZIP="$CACHE_DIR/ninja-linux.zip"
  if [ ! -f "$NINJA_ZIP" ]; then
    download \
      "https://github.com/ninja-build/ninja/releases/download/v$NINJA_VERSION/ninja-linux.zip" \
      "$NINJA_ZIP"
  fi
  mkdir -p "$CACHE_DIR/ninja"
  python3 -c "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])" \
    "$NINJA_ZIP" "$CACHE_DIR/ninja"
  chmod +x "$CACHE_DIR/ninja/ninja"
fi

export PATH="$CACHE_DIR/cmake/bin:$CACHE_DIR/ninja:$PATH"
echo "cmake: $(command -v cmake) ($(cmake --version | head -n1))"
echo "ninja: $(command -v ninja) ($(ninja --version))"

echo "==> Setting up Emscripten $EMSDK_VERSION in $EMSDK_DIR..."
if [ ! -d "$EMSDK_DIR" ]; then
  mkdir -p "$(dirname "$EMSDK_DIR")"
  git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
fi

cd "$EMSDK_DIR"
./emsdk install "$EMSDK_VERSION"
./emsdk activate "$EMSDK_VERSION"
# shellcheck disable=SC1091
source ./emsdk_env.sh
cd "$ROOT"

echo "==> Building WASM..."
emcmake cmake -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release
emmake ninja -C "$BUILD_DIR" -j"$(nproc 2>/dev/null || echo 4)"

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

# Required headers (replaces nginx.conf COOP/COEP).
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
