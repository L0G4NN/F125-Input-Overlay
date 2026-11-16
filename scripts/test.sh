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
"${CXX:-g++}" -std=c++20 -Wall -Wextra -Werror -fsanitize=address,undefined -Isrc tests/receive_gate_test.cpp -o build/receive_gate_test
./build/receive_gate_test
"${CXX:-g++}" -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc tests/lap_review_test.cpp -o build/lap_review_test
./build/lap_review_test
"${CXX:-g++}" -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc -Ithird_party/swofty tests/review_panel_test.cpp -o build/review_panel_test
./build/review_panel_test
"${CXX:-g++}" -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc tests/hud_visibility_test.cpp -o build/hud_visibility_test
./build/hud_visibility_test
"${CXX:-g++}" -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer -Isrc tests/extended_test.cpp -o build/extended_test
./build/extended_test
"${CXX:-g++}" -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer -Isrc tests/multi_review_test.cpp -o build/multi_review_test
./build/multi_review_test
