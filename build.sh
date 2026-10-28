#!/usr/bin/env bash
#
# build.sh - Build PicoCalc Frotz release images
#
# Usage: ./build.sh [--clean] [board...]
#
# Builds each board (default: pico pico2) and copies the images to
# dist/picocalc-frotz-v<version>-<board>.uf2, where <version> comes from the
# git tags (e.g. v1.2 on a release tag, v1.2-3-gabc1234-dirty otherwise).
#
# Uses the SDK, toolchain and picotool versions named in CMakeLists.txt from
# ~/.pico-sdk (installed by the Raspberry Pi Pico VS Code extension) when
# present; otherwise PICO_SDK_PATH and the tools on PATH are used.
#

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PICO_HOME="${HOME}/.pico-sdk"
DIST="${ROOT}/dist"

clean=0
boards=()
for arg in "$@"; do
    case "$arg" in
        --clean) clean=1 ;;
        -h|--help) sed -n '3,14p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        -*) echo "Unknown option: $arg" >&2; exit 1 ;;
        *) boards+=("$arg") ;;
    esac
done
[ ${#boards[@]} -eq 0 ] && boards=(pico pico2)

# Read a version from the VS Code extension block in CMakeLists.txt
cmake_setting() {
    sed -n "s/^set($1 \(.*\))$/\1/p" "${ROOT}/CMakeLists.txt"
}

# Newest version directory in ~/.pico-sdk/<tool>, if any
newest() {
    ls -d "${PICO_HOME}/$1"/*/ 2>/dev/null | sort -V | tail -1 | sed 's:/$::'
}

if [ -d "$PICO_HOME" ]; then
    sdk="${PICO_HOME}/sdk/$(cmake_setting sdkVersion)"
    toolchain="${PICO_HOME}/toolchain/$(cmake_setting toolchainVersion)"
    picotool="${PICO_HOME}/picotool/$(cmake_setting picotoolVersion)"
    for dir in "$sdk" "$toolchain" "$picotool"; do
        if [ ! -d "$dir" ]; then
            echo "Missing ${dir}; install it with the Raspberry Pi Pico VS Code extension" >&2
            exit 1
        fi
    done
    export PICO_SDK_PATH="$sdk"
    export PICO_TOOLCHAIN_PATH="$toolchain"
    export picotool_DIR="${picotool}/picotool"
    PATH="${toolchain}/bin:${picotool}/picotool:${PATH}"
    cmake_dir="$(newest cmake)"; [ -n "$cmake_dir" ] && PATH="${cmake_dir}/bin:${PATH}"
    ninja_dir="$(newest ninja)"; [ -n "$ninja_dir" ] && PATH="${ninja_dir}:${PATH}"
    export PATH
elif [ -z "${PICO_SDK_PATH:-}" ]; then
    echo "Set PICO_SDK_PATH or install the SDK with the Raspberry Pi Pico VS Code extension" >&2
    exit 1
fi

if [ ! -f "${ROOT}/modules/frotz/src/common/frotz.h" ]; then
    echo "Fetching submodules"
    git -C "$ROOT" submodule update --init --recursive
fi

version="$(git -C "$ROOT" describe --tags --dirty --match 'v[0-9]*' 2>/dev/null || echo unknown)"
mkdir -p "$DIST"

for board in "${boards[@]}"; do
    build="${ROOT}/build/release-${board}"
    echo "=== Building ${board} (${version})"
    [ $clean -eq 1 ] && rm -rf "$build"
    # Always configure so the version is picked up from the latest git tag
    cmake -G Ninja -S "$ROOT" -B "$build" -DPICO_BOARD="$board" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$build"
    cp "${build}/picocalc-frotz.uf2" "${DIST}/picocalc-frotz-${version}-${board}.uf2"
done

echo
echo "Images:"
for board in "${boards[@]}"; do
    echo "  dist/picocalc-frotz-${version}-${board}.uf2"
done
