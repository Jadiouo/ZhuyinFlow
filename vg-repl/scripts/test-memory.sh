#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
# SPDX-License-Identifier: MulanPSL-2.0
set -euo pipefail
if [[ $# -ne 2 ]]; then
  echo "Usage: $0 BRIDGE_BIN_DIRECTORY ARTIFACT_DIRECTORY" >&2
  exit 2
fi
repo_root=$(cd "$(dirname "$0")/../.." && pwd)
bridge_bin=$(realpath "$1")
artifacts=$(realpath -m "$2")
mkdir -p "$artifacts"
cd "$repo_root"
export LD_LIBRARY_PATH="$bridge_bin${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export ASAN_OPTIONS=detect_leaks=1
suppression="$repo_root/vg-repl/Tests/c/corelibs-cfrunarray.supp"
clang -fsanitize=address -g -I vg-repl/Sources/VGBridge/include \
  vg-repl/Tests/c/test_vgbridge.c -L "$bridge_bin" -lvgbridge \
  -Wl,-rpath,"$bridge_bin" -o "$artifacts/test_vgbridge" \
  > "$artifacts/compile.log" 2>&1
# Preserve the complete unsuppressed verdict as a diagnostic. A known Foundation
# leak must not prevent independent functional/headless checks from running.
test_home=$(mktemp -d)
trap 'rm -rf "$test_home"' EXIT
set +e
HOME="$test_home" XDG_CONFIG_HOME="$test_home/.config" LSAN_OPTIONS= \
  "$artifacts/test_vgbridge" > "$artifacts/strict.stdout" 2> "$artifacts/strict.stderr.log"
strict_status=$?
set -e
printf '%s\n' "$strict_status" > "$artifacts/strict.exit"
printf 'Unsuppressed LSan exit: %s (complete output: strict.stderr.log)\n' "$strict_status"
# This required run excludes only the verified Foundation allocation frame.
export LSAN_OPTIONS="suppressions=$suppression:print_suppressions=1"
bash vg-repl/Tests/c/test_output_contract.sh "$artifacts/test_vgbridge" \
  vg-repl/Tests/Fixtures/fcitx-su3.jsonl "$artifacts/required"

swiftc -sanitize=address -g vg-repl/Tests/c/foundation_repeat.swift \
  -o "$artifacts/foundation_repeat" > "$artifacts/foundation-compile.log" 2>&1
for count in 1 10 100; do
  set +e
  LSAN_OPTIONS= "$artifacts/foundation_repeat" empty "$count" \
    > "$artifacts/foundation-$count.stdout" 2> "$artifacts/foundation-$count.stderr.log"
  foundation_status=$?
  set -e
  printf '%s\n' "$foundation_status" > "$artifacts/foundation-$count.exit"
  grep -F 'objectReleased=true' "$artifacts/foundation-$count.stdout" > /dev/null
done

# Invert the controls: intentional leaks must fail even with the suppression.
for control in malloc response; do
  clang -fsanitize=address -g -O0 -I vg-repl/Sources/VGBridge/include \
    "vg-repl/Tests/c/lsan_${control}_control.c" -L "$bridge_bin" -lvgbridge \
    -Wl,-rpath,"$bridge_bin" -o "$artifacts/control-$control" \
    > "$artifacts/control-$control-compile.log" 2>&1
  set +e
  HOME="$test_home" XDG_CONFIG_HOME="$test_home/.config" \
    "$artifacts/control-$control" > "$artifacts/control-$control.stdout" \
      2> "$artifacts/control-$control.stderr.log"
  control_status=$?
  set -e
  printf '%s\n' "$control_status" > "$artifacts/control-$control.exit"
  test "$control_status" -eq 1
  grep -F 'ERROR: LeakSanitizer: detected memory leaks' \
    "$artifacts/control-$control.stderr.log" > /dev/null
  if [[ "$control" == malloc ]]; then
    grep -F '41 byte(s) leaked' "$artifacts/control-$control.stderr.log" > /dev/null
  else
    grep -F ' in strdup ' "$artifacts/control-$control.stderr.log" > /dev/null
    grep -F 'vg_feed_key' "$artifacts/control-$control.stderr.log" > /dev/null
    response_length=$(sed -n 's/^responseLength=\([0-9][0-9]*\) intentionally not freed$/\1/p' "$artifacts/control-$control.stderr.log")
    test -n "$response_length"
    grep -F "$((response_length + 1)) byte(s) leaked" "$artifacts/control-$control.stderr.log" > /dev/null
  fi
done
# Classify raw failures after running the independent required checks and controls.
bash vg-repl/scripts/classify-lsan.sh "$strict_status" "$artifacts/strict.stderr.log" \
  > "$artifacts/strict-classification.log" 2>&1
for count in 1 10 100; do
  bash vg-repl/scripts/classify-lsan.sh "$(cat "$artifacts/foundation-$count.exit")" \
    "$artifacts/foundation-$count.stderr.log" > "$artifacts/foundation-$count-classification.log" 2>&1
done
printf 'Required narrow-suppressed LSan, strict classification, and both intentional leak controls passed.\n'
