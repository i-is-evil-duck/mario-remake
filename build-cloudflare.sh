#!/usr/bin/env bash
set -euo pipefail

# Cloudflare Workers Builds: compile WASM with Emscripten, output to ./public.
# Dashboard settings:
#   Build command:   npm run build
#   Deploy command:  npx wrangler deploy

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PUBLIC_DIR="$ROOT/public"
BUILD_DIR="$ROOT/build_wasm"
EMSDK_DIR="${EMSDK_DIR:-$ROOT/.cache/emsdk}"
EMSDK_VERSION="3.1.74"

echo "==> Checking tools..."
for t in git python3 cmake ninja; do
  if ! command -v "$t" >/dev/null 2>&1; then
    echo "Missing required tool: $t"
    if command -v apt-get >/dev/null 2>&1; then
      echo "Trying apt-get install..."
      apt-get update && apt-get install -y git python3 cmake ninja-build
    else
      echo "Please install $t first." >&2
      exit 1
    fi
  fi
done

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
