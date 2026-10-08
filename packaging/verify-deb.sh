#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
# SPDX-License-Identifier: MulanPSL-2.0
set -euo pipefail
[[ $# -ge 1 && $# -le 2 ]] || { echo "Usage: $0 DEB [ARTIFACT_DIRECTORY]" >&2; exit 2; }
repo=$(cd "$(dirname "$0")/.." && pwd)
deb=$(realpath "$1")
work=$(realpath -m "${2:-$repo/.scratch/package-2026-10-08/verify}")
mkdir -p "$work"
docker run --rm --network=none --user "$(id -u):$(id -g)" \
  -v "$repo:/source:ro" -v "$work:/verify" "${ZHUYINFLOW_PACKAGE_IMAGE:-zhuyinflow-package:noble}" \
  bash -c 'g++ -std=c++17 -O2 -DNDEBUG /source/packaging/installed-smoke.cpp $(pkg-config --cflags --libs Fcitx5Core) -o /verify/installed-smoke'
# This container never sees the host HOME, desktop, session bus, or ~/.local.
docker run --rm -v "$repo:/source:ro" -v "$deb:/package/zhuyinflow.deb:ro" \
  -v "$work:/verify" ubuntu:24.04 bash /source/packaging/container-verify.sh
