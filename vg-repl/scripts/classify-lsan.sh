#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
# SPDX-License-Identifier: MulanPSL-2.0
set -euo pipefail
if [[ $# -ne 2 ]]; then
  echo "Usage: $0 EXIT_STATUS STDERR_LOG" >&2
  exit 2
fi
status=$1
log=$2
if [[ "$status" == 0 ]]; then
  if grep -E 'ERROR:|SUMMARY: AddressSanitizer:|UndefinedBehaviorSanitizer|AddressSanitizer:DEADLYSIGNAL|runtime error:' "$log" > /dev/null; then
    echo 'Successful exit contained a sanitizer error.' >&2
    exit 1
  fi
  echo 'No unsuppressed leak detected; review whether the Foundation suppression is still needed.'
  exit 0
fi
if [[ "$status" != 1 ]] || \
  grep -E 'ERROR: AddressSanitizer:|UndefinedBehaviorSanitizer|AddressSanitizer:DEADLYSIGNAL|runtime error:|Signal [0-9]+|Program crashed:|Assertion.*failed' "$log" > /dev/null; then
  echo "Unclassified sanitizer/process failure (exit $status)." >&2
  exit 1
fi
grep -F 'ERROR: LeakSanitizer: detected memory leaks' "$log" > /dev/null
grep -E '^SUMMARY: AddressSanitizer: [0-9]+ byte\(s\) leaked in [0-9]+ allocation\(s\)\.' "$log" > /dev/null
# Every direct allocation root must contain the exact known Foundation function.
# Indirect children are classified by their direct roots, as LSan suppression is.
awk '
  /^Direct leak of / {
    if (direct && !known) invalid = 1;
    direct = 1; known = 0; roots++; next;
  }
  /^Indirect leak of / {
    if (direct && !known) invalid = 1;
    direct = 0; next;
  }
  direct && / in CFRunArrayCreate([[:space:]]|$)/ { known = 1; }
  END {
    if (direct && !known) invalid = 1;
    if (!roots || invalid) exit 1;
  }
' "$log"
echo 'Known Swift Linux Foundation CFRunArray leak; the runtime defect remains unfixed.'
