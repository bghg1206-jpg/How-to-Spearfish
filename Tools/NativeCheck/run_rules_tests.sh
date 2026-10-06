#!/usr/bin/env bash
# Builds and runs the rules-layer tests natively (no Unreal install needed).
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
MODULE="$ROOT/Source/HowToSpearfish"
BUILD="$HERE/.build/rules"
mkdir -p "$BUILD/gen"
python3 "$HERE/gen_uht.py" "$MODULE" "$BUILD/gen" > "$BUILD/gen.log" || { cat "$BUILD/gen.log"; echo "(gen_uht reported problems; continuing with rules tests)"; }
CXX="${CXX:-clang++}"
"$CXX" -std=c++20 -O1 -g -Wall -Wextra -Wno-unused-parameter -Werror=return-type \
  -I"$HERE/stub" -I"$BUILD/gen" -I"$MODULE" \
  "$MODULE"/Rules/*.cpp "$MODULE"/World/SpearfishTerrain.cpp "$HERE/RulesTestMain.cpp" -o "$BUILD/rules_tests"
"$BUILD/rules_tests"
