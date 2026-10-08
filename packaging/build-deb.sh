#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
# SPDX-License-Identifier: MulanPSL-2.0
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
version=${ZHUYINFLOW_VERSION:-0.1.0}
[[ $version =~ ^[0-9]+\.[0-9]+\.[0-9]+([+~.-][a-zA-Z0-9.+~_-]+)?$ ]] || { echo 'Invalid Debian version' >&2; exit 2; }
lexicon_revision=d41f2fc244eadf94c37df50ef98e716fdc28146d
upstream_revision=977a05fe353bcc43cffbc03ea8b6d815a072fd65
work=${ZHUYINFLOW_PACKAGE_WORK:-$repo/.scratch/package-2026-10-08/build}
output=${ZHUYINFLOW_PACKAGE_OUTPUT:-$repo/dist}
image=${ZHUYINFLOW_PACKAGE_IMAGE:-zhuyinflow-package:noble}
swift_image=${SWIFT_IMAGE:-swift:6.4-noble}
lexicon=${ZHUYINFLOW_LEXICON_SOURCE:-${XDG_CACHE_HOME:-$HOME/.cache}/zhuyinflow/VanguardLexicon-$lexicon_revision}
mkdir -p "$work" "$output"
work=$(realpath "$work"); output=$(realpath "$output")
if [[ ! -d "$lexicon/.git" ]]; then
  mkdir -p "$(dirname "$lexicon")"
  git clone https://github.com/vChewing/vChewing-VanguardLexicon.git "$lexicon"
  git -C "$lexicon" checkout --detach "$lexicon_revision"
fi
[[ $(git -C "$lexicon" rev-parse HEAD) == "$lexicon_revision" ]]
[[ -z $(git -C "$lexicon" status --porcelain) ]] || { echo 'Lexicon source must be clean' >&2; exit 1; }
[[ $(git -C "$repo/upstream" rev-parse HEAD) == "$upstream_revision" ]]
[[ -z $(git -C "$repo/upstream" status --porcelain) ]] || { echo 'Upstream source must be clean' >&2; exit 1; }
# Snapshot the current source, including pending fixes, but no ignored scratch,
# user configuration, build products, or Git history. Submodules get exact exports.
[[ ! -e "$work/source" ]] || { echo "Use a fresh work directory: $work" >&2; exit 1; }
mkdir "$work/source"
git -C "$repo" ls-files -z --cached --others --exclude-standard > "$work/files.zlist"
python3 - "$repo" "$work/source" "$work/files.zlist" <<'PY'
import pathlib, shutil, sys
root, target, listing = map(pathlib.Path, sys.argv[1:])
for name in listing.read_bytes().split(b'\0'):
    if not name: continue
    name = name.decode()
    if name == 'upstream': continue
    p = pathlib.PurePosixPath(name)
    if p.is_absolute() or '..' in p.parts: raise RuntimeError(name)
    src, dst = root / name, target / name
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst, follow_symlinks=False)
PY
mkdir "$work/source/upstream" "$work/source/lexicon-source"
git -C "$repo/upstream" archive "$upstream_revision" | tar -x -C "$work/source/upstream"
# An unused Darwin-only prebuilt archive from Xcode is not a Linux build input
# and has no established binary redistribution grant. Keep the source/notice.
rm "$work/source/upstream/LegacyZone/ARCLite/libarclite_macosx.a"
git -C "$lexicon" archive "$lexicon_revision" | tar -x -C "$work/source/lexicon-source"
epoch=${SOURCE_DATE_EPOCH:-$(git -C "$lexicon" show -s --format=%ct "$lexicon_revision")}
printf 'version=%s\nupstream=%s\nlexicon=%s\nsource_date_epoch=%s\nexcluded=upstream/LegacyZone/ARCLite/libarclite_macosx.a (unused Darwin prebuilt archive)\n' "$version" "$upstream_revision" "$lexicon_revision" "$epoch" > "$work/source/BUILD-PROVENANCE"
printf 'root_revision=%s\n' "$(git -C "$repo" rev-parse HEAD)" >> "$work/source/BUILD-PROVENANCE"
if [[ -n $(git -C "$repo" status --porcelain) ]]; then
  echo 'root_dirty=true (SOURCE-SHA256 describes the actual build tree)' >> "$work/source/BUILD-PROVENANCE"
else
  echo 'root_dirty=false' >> "$work/source/BUILD-PROVENANCE"
fi
# BuildKit may cache a FROM image without importing its tag into the daemon.
# Resolve before building so cold runners can inspect the actual base image.
if ! docker image inspect "$swift_image" >/dev/null 2>&1; then
  docker pull "$swift_image"
fi
swift_build_base=$(docker image inspect "$swift_image" --format '{{if .RepoDigests}}{{index .RepoDigests 0}}{{end}}')
swift_build_base=${swift_build_base:-$swift_image}
docker image inspect "$swift_image" --format '{{.Id}}{{range .RepoDigests}} {{.}}{{end}}' > "$work/source/SWIFT-IMAGE"
docker build --build-arg "SWIFT_IMAGE=$swift_build_base" -t "$image" "$repo/packaging"
docker image inspect "$image" --format '{{.Id}}' > "$work/build-image.txt"
docker run --rm --network=none "$image" dpkg-query -W > "$work/source/BUILD-PACKAGES"
(cd "$work/source" && find . -type f ! -name SOURCE-SHA256 -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256)
docker run --rm --network=none --user "$(id -u):$(id -g)" \
  -e "SOURCE_DATE_EPOCH=$epoch" -e "ZHUYINFLOW_VERSION=$version" \
  -v "$work/source:/source:ro" -v "$work:/build" -v "$output:/out" \
  "$image" bash /source/packaging/container-build.sh
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner \
  -cJf "$output/zhuyinflow_${version}_sources.tar.xz" -C "$work/source" .
(cd "$output" && sha256sum "zhuyinflow_${version}_amd64.deb" "zhuyinflow_${version}_sources.tar.xz") > "$output/zhuyinflow_${version}_SHA256SUMS"
cat "$output/zhuyinflow_${version}_SHA256SUMS"
