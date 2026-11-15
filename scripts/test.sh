#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
"${CXX:-g++}" -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror \
    -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc tests/trace_test.cpp -o build/trace_test
./build/trace_test

"${CXX:-g++}" -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc tests/udp_test.cpp -o build/udp_test
./build/udp_test
"${CXX:-g++}" -std=c++20 -Wall -Wextra -Werror -Isrc tests/layout_test.cpp -o build/layout_test
./build/layout_test
