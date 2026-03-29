#!/bin/bash
set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"
SRC="$ROOT/src"
INC="$ROOT/include"
OUT="$ROOT/hydronaut"

echo "═══════════════════════════════════════"
echo "  Building Hydronaut"
echo "═══════════════════════════════════════"

mkdir -p build
  cd build
  cmake -DCMAKE_PREFIX_PATH="$ROOT/libtorch" ..
  cmake --build . --target hydronaut -j$(nproc)
  cp hydronaut "$OUT"

echo "Build successful → $OUT"
echo ""
echo "Running Hydronaut..."
cd "$ROOT"
"$OUT"