#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
# SPDX-License-Identifier: MulanPSL-2.0
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "Usage: $0 HARNESS GOLDEN ARTIFACT_PREFIX" >&2
  exit 2
fi
harness=$1
golden=$2
artifact_prefix=$3
# Keep upstream debug defaults predictable without modifying personal prefs.
test_home=$(mktemp -d)
trap 'rm -rf "$test_home"' EXIT
HOME="$test_home" XDG_CONFIG_HOME="$test_home/.config" \
  "$harness" > "${artifact_prefix}.jsonl" 2> "${artifact_prefix}.stderr.log"
# Compare every stdout byte: no line selection or non-JSON filtering.
diff -u "$golden" "${artifact_prefix}.jsonl"
# Loading diagnostics must survive on stderr, including with Release libraries.
grep -F 'Factory TextMap loading complete:' "${artifact_prefix}.stderr.log" > /dev/null
