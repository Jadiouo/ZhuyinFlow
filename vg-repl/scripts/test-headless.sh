#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
# SPDX-License-Identifier: MulanPSL-2.0
set -euo pipefail
if [[ $# -ne 3 ]]; then
  echo "Usage: $0 BRIDGE_BIN_DIRECTORY BUILD_DIRECTORY Debug|Release" >&2
  exit 2
fi
repo_root=$(cd "$(dirname "$0")/../.." && pwd)
bridge_bin=$(realpath "$1")
build_directory=$(realpath -m "$2")
configuration=$3
case "$configuration" in Debug|Release) ;; *) exit 2 ;; esac
mkdir -p "$build_directory"
cmake -S "$repo_root/vg-repl/fcitx5" -B "$build_directory" \
  -DCMAKE_BUILD_TYPE="$configuration" \
  -DVG_BRIDGE_LIBRARY="$bridge_bin/libvgbridge.so" \
  -DVG_VANGUARD_LIBRARY="$bridge_bin/libVanguard.so" \
  -DVG_TEST_TEXTMAP="$repo_root/upstream/Packages/vChewing_OSNeutral_LibVanguard/Sources/LXAssemblyMaterials4Tests/Resources/vanguardTextMap_test.txtMap" \
  > "$build_directory/configure.log" 2>&1
cmake --build "$build_directory" --parallel 2 > "$build_directory/build.log" 2>&1
# CMake gives each TestFrontend process its own HOME/XDG/FCITX paths and disables
# desktop displays and the session bus. No install or live daemon is involved.
LD_LIBRARY_PATH="$bridge_bin${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
  ctest --test-dir "$build_directory" --output-on-failure \
    --output-junit "$build_directory/ctest.xml" > "$build_directory/ctest.log" 2>&1
cat "$build_directory/ctest.log"
