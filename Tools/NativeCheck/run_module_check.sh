#!/usr/bin/env bash
# Type-checks the whole game module natively (no Unreal Engine needed):
#   1. gen_uht.py writes *.generated.h stand-ins and validates UHT rules
#   2. every engine include used by the module is mapped to the declaration-level engine stub
#   3. clang -fsyntax-only on every .cpp, plus every header compiled on its own (self-containment)
# This catches mismatches between our own classes, wrong engine call signatures (as far as the stub mirrors
# UE 5.5) and missing includes of our own headers. It is not a substitute for a real UnrealBuildTool build.
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MODULE="$ROOT/Source/HowToSpearfish"
STUB="$ROOT/Tools/NativeCheck/stub"
BUILD="${BUILD_DIR:-$ROOT/Intermediate/NativeCheck}"
CXX="${CXX:-clang++}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"

rm -rf "$BUILD/uht" "$BUILD/engine" "$BUILD/logs" "$BUILD/headers"
mkdir -p "$BUILD/uht" "$BUILD/engine" "$BUILD/logs" "$BUILD/headers"

python3 "$ROOT/Tools/NativeCheck/gen_uht.py" "$MODULE" "$BUILD/uht" || UHT_FAILED=1

# Engine includes: anything that is not one of our module folders, CoreMinimal or a generated header.
PROJECT_DIRS="$(cd "$MODULE" && find . -mindepth 1 -maxdepth 1 -type d | sed 's|^\./||' | paste -sd'|')"
grep -rhoE '#include "[^"]+"' --include=*.h --include=*.cpp "$MODULE" | sed 's/#include "//;s/"$//' | sort -u | while read -r INC; do
	case "$INC" in
		*.generated.h|CoreMinimal.h|HowToSpearfish.h) continue ;;
	esac
	if echo "$INC" | grep -qE "^($PROJECT_DIRS)/"; then
		continue
	fi
	mkdir -p "$BUILD/engine/$(dirname "$INC")"
	printf '#pragma once\n#include "UEStub/UEStubAll.h"\n' > "$BUILD/engine/$INC"
done

# Warnings approximate UnrealBuildTool's clang toolchain (-Wall -Werror, shadowing as error in UE5, plus the
# warnings UBT turns off). Stub, engine and generated headers are system headers so only our code is judged.
WARNINGS=(-Wall -Wshadow-all -Wdelete-non-virtual-dtor -Wenum-conversion -Wbitfield-enum-conversion
	-Wno-unknown-warning-option -Wno-unused-but-set-variable -Wno-unused-but-set-parameter -Wno-deprecated-copy
	-Wno-gnu-string-literal-operator-template -Wno-inconsistent-missing-override -Wno-invalid-offsetof -Wno-switch
	-Wno-tautological-compare -Wno-unknown-pragmas -Wno-unused-function -Wno-unused-lambda-capture
	-Wno-unused-local-typedef -Wno-unused-private-field -Wno-unused-variable -Wno-undefined-var-template)
[ "${NO_WERROR:-0}" = 1 ] || WARNINGS+=(-Werror)
FLAGS=(-std=c++20 -fsyntax-only "${WARNINGS[@]}"
	-DWITH_DEV_AUTOMATION_TESTS=1 -isystem "$STUB" -isystem "$BUILD/engine" -isystem "$BUILD/uht" -I"$MODULE")

export CXX
FLAGS_STR="$(printf '%q ' "${FLAGS[@]}")"

FAILS=0
mapfile -t CPPS < <(cd "$MODULE" && find . -name '*.cpp' | sed 's|^\./||' | sort)
mapfile -t HEADERS < <(cd "$MODULE" && find . -name '*.h' | sed 's|^\./||' | sort)

run_one() {
	local REL="$1" KIND="$2"
	local LOG="$BUILD/logs/$(echo "$REL" | tr '/' '_').$KIND.log"
	local SRC="$MODULE/$REL"
	if [ "$KIND" = header ]; then
		SRC="$BUILD/headers/$(echo "$REL" | tr '/' '_').cpp"
		printf '#include "%s"\n#include "%s"\n' "$REL" "$REL" > "$SRC"
	fi
	if ! eval "$CXX $FLAGS_STR \"$SRC\"" >"$LOG" 2>&1; then
		echo "FAIL [$KIND] $REL"
	else
		rm -f "$LOG"
	fi
}
export -f run_one
export BUILD MODULE FLAGS_STR

RESULTS="$( (for F in "${CPPS[@]}"; do echo "$F cpp"; done; for F in "${HEADERS[@]}"; do echo "$F header"; done) |
	xargs -P "$JOBS" -n 2 bash -c 'run_one "$0" "$1"')"
echo "$RESULTS" | sed '/^$/d' | sort
FAILS=$(echo "$RESULTS" | grep -c '^FAIL' || true)
ERRORS=$(cat "$BUILD"/logs/*.log 2>/dev/null | grep -cE 'error:|warning:' || true)
python3 "$ROOT/Tools/NativeCheck/lint_shadow.py" "$CXX" "$MODULE" "${FLAGS[@]}" || SHADOW_FAILED=1
python3 "$ROOT/Tools/NativeCheck/lint_random.py" "$MODULE" || RANDOM_FAILED=1
echo "module check: ${#CPPS[@]} sources, ${#HEADERS[@]} headers, $FAILS failing unit(s), $ERRORS diagnostic(s). Logs: $BUILD/logs"
[ "$FAILS" -eq 0 ] && [ -z "${UHT_FAILED:-}" ] && [ -z "${SHADOW_FAILED:-}" ] && [ -z "${RANDOM_FAILED:-}" ]
