#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
# SPDX-License-Identifier: MulanPSL-2.0
set -euo pipefail
source_root=$(cd "$(dirname "$0")/.." && pwd)
test -s "$source_root/BUILD-PROVENANCE"
test -d "$source_root/lexicon-source"
(cd "$source_root" && sha256sum --check SOURCE-SHA256)
version=$(sed -n 's/^version=//p' "$source_root/BUILD-PROVENANCE")
epoch=$(sed -n 's/^source_date_epoch=//p' "$source_root/BUILD-PROVENANCE")
[[ $version =~ ^[0-9]+\.[0-9]+\.[0-9]+([+~.-][a-zA-Z0-9.+~_-]+)?$ && $epoch =~ ^[0-9]+$ ]]
work=$(realpath -m "${ZHUYINFLOW_PACKAGE_WORK:-$source_root/.scratch/rebuild}")
output=$(realpath -m "${ZHUYINFLOW_PACKAGE_OUTPUT:-$source_root/dist}")
[[ ! -e "$work" ]] || { echo 'Use a fresh work directory' >&2; exit 1; }
mkdir -p "$work" "$output"
image=${ZHUYINFLOW_PACKAGE_IMAGE:-zhuyinflow-package:noble}
docker build --build-arg "SWIFT_IMAGE=${SWIFT_IMAGE:-swift:6.4-noble}" -t "$image" "$source_root/packaging"
docker run --rm --network=none --user "$(id -u):$(id -g)" \
  -e "SOURCE_DATE_EPOCH=$epoch" -e "ZHUYINFLOW_VERSION=$version" \
  -v "$source_root:/source:ro" -v "$work:/build" -v "$output:/out" \
  "$image" bash /source/packaging/container-build.sh
