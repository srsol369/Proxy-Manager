#!/usr/bin/env bash
# Run inside MSYS2 UCRT64 (not the MSYS posix shell).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

if [[ "$(uname -o 2>/dev/null || true)" == "Msys" ]] && [[ -x /ucrt64/bin/g++ ]]; then
  export PATH="/ucrt64/bin:$PATH"
fi

TRIPLE="$(g++ -dumpmachine)"
if [[ "$TRIPLE" != *mingw* ]]; then
  echo "Wrong compiler: $TRIPLE"
  echo "Open 'MSYS2 UCRT64' from the Start menu, or:"
  echo "  pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make"
  echo "  export PATH=/ucrt64/bin:\$PATH"
  exit 1
fi

if [[ ! -f third_party/imgui/imgui.cpp ]]; then
  git clone --depth 1 --branch v1.91.8 https://github.com/ocornut/imgui.git third_party/imgui
fi

if command -v mingw32-make >/dev/null; then
  mingw32-make -j"$(nproc)"
else
  make -j"$(nproc)"
fi

echo "Built: $ROOT/build-msys/NetProxyManager.exe"
