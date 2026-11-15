#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${ZIG:=zig}"
[[ "$("$ZIG" version)" == "0.14.1" ]] || { echo "Zig 0.14.1 required" >&2; exit 1; }
mkdir -p build/native
"$ZIG" c++ -target x86_64-windows-gnu -std=c++20 -O2 -Wall -Wextra -Werror \
    -Isrc -Ithird_party/swofty -shared -static -s src/swofty/plugin.cpp -lws2_32 \
    -o build/native/input_overlay.dll

cat > build/BUILD-INFO.txt <<'INFO'
F1 25 Input Overlay 0.1.0 experimental
Target: x86_64-windows-gnu
Compiler requirement: Zig 0.14.1, C++20, -O2
SDK: SwoftyLauncher 5d538d9a0afd6f90a417118807d86a5beb9e9c71
Input: EA UDP format 2025, Car Telemetry packet 6 version 1
No in-game validation performed.
INFO
