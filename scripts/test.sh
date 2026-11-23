#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build

flags=(-std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror
       -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer
       -Isrc -Ithird_party/swofty)

for source in tests/*_test.cpp; do
    target="build/$(basename "${source%.cpp}")"
    "${CXX:-g++}" "${flags[@]}" "$source" -o "$target"
    "$target"
done
