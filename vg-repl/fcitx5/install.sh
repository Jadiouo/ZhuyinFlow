#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
# SPDX-License-Identifier: MulanPSL-2.0
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
lexicon_commit="d41f2fc244eadf94c37df50ef98e716fdc28146d"
lexicon_cache="${XDG_CACHE_HOME:-$HOME/.cache}/zhuyinflow/VanguardLexicon-$lexicon_commit"
data_root="${XDG_DATA_HOME:-$HOME/.local/share}/zhuyinflow"
text_map="$data_root/Build/Release/vanguard-textmap/VanguardFactoryDict4Typing.txtMap"
addon_dir="${XDG_DATA_HOME:-$HOME/.local/share}/fcitx5/addon"
inputmethod_dir="${XDG_DATA_HOME:-$HOME/.local/share}/fcitx5/inputmethod"
library_dir="${HOME}/.local/lib/fcitx5"
swift_image="${ZHUYINFLOW_SWIFT_IMAGE:-swift:6.4-noble}"

for tool in docker cmake pkg-config git; do
  command -v "$tool" >/dev/null || {
    printf 'Required tool not found: %s\n' "$tool" >&2
    exit 1
  }
done

mkdir -p "$(dirname "$lexicon_cache")" "$data_root" "$addon_dir" \
  "$inputmethod_dir" "$library_dir"

if [[ ! -d "$lexicon_cache/.git" ]]; then
  git clone --quiet https://github.com/vChewing/vChewing-VanguardLexicon.git \
    "$lexicon_cache"
  git -C "$lexicon_cache" checkout --quiet --detach "$lexicon_commit"
elif [[ "$(git -C "$lexicon_cache" rev-parse HEAD)" != "$lexicon_commit" ]]; then
  printf 'Lexicon cache has an unexpected revision: %s\n' "$lexicon_cache" >&2
  exit 1
fi

if [[ ! -s "$text_map" || "${ZHUYINFLOW_REBUILD_TEXTMAP:-0}" == "1" ]]; then
  docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp \
    -e VANGUARD_OUTPUT_DIR=/out \
    -v "$lexicon_cache:/lexicon" -v "$data_root:/out" -w /lexicon \
    "$swift_image" swift run -c release VCDataBuilder vanguardTextMap
fi

build_dir="$repo_root/vg-repl/.build/fcitx5-noble"
docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$repo_root:/work" -w /work "$swift_image" \
  swift build --package-path /work/vg-repl --scratch-path /work/vg-repl/.build/fcitx5-noble/swift \
    -c release --product vgbridge
container_bin_path="$(
  docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp \
    -v "$repo_root:/work" -w /work "$swift_image" \
    swift build --package-path /work/vg-repl --scratch-path /work/vg-repl/.build/fcitx5-noble/swift \
      -c release --show-bin-path
)"
host_bin_path="$repo_root${container_bin_path#/work}"

cmake -S "$repo_root/vg-repl/fcitx5" -B "$build_dir/cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DVG_BRIDGE_LIBRARY="$host_bin_path/libvgbridge.so" \
  -DVG_VANGUARD_LIBRARY="$host_bin_path/libVanguard.so" \
  -DVG_INSTALL_ADDON_DIR="$library_dir" \
  -DVG_INSTALL_DATA_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/fcitx5" \
  -DVG_INSTALL_ICON_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/icons/hicolor/scalable/apps"
cmake --build "$build_dir/cmake" --parallel
cmake --install "$build_dir/cmake"

docker run --rm --user "$(id -u):$(id -g)" \
  -v "$host_bin_path:/build" -v "$library_dir:/runtime" "$swift_image" \
  bash -euo pipefail -c '
    for library in /build/libvgbridge.so /build/libVanguard.so; do
      test -f "$library"
      for dependency in $(ldd "$library" |
        awk '"'"'$2 == "=>" && $3 ~ /^\/usr\/lib\/swift\/linux\// { print $3 }'"'"'); do
        cp -L "$dependency" /runtime/
      done
    done
  '

install -m 0644 "$text_map" "$data_root/VanguardFactoryDict4Typing.txtMap"
icon_theme_dir="${XDG_DATA_HOME:-$HOME/.local/share}/icons/hicolor"
if [[ ! -f "$icon_theme_dir/index.theme" && -f /usr/share/icons/hicolor/index.theme ]]; then
  install -D -m 0644 /usr/share/icons/hicolor/index.theme "$icon_theme_dir/index.theme"
fi
if command -v gtk-update-icon-cache >/dev/null && [[ -f "$icon_theme_dir/index.theme" ]]; then
  gtk-update-icon-cache --force "$icon_theme_dir"
fi
ctest --test-dir "$build_dir/cmake" --output-on-failure
runtime_report="$(ldd "$library_dir/libvgbridge.so" 2>&1)"
if grep -Eq 'not found|version .* not found' <<<"$runtime_report"; then
  printf 'Swift runtime dependencies are not compatible:\n%s\n' "$runtime_report" >&2
  exit 1
fi

printf '\nInstalled ZhuyinFlow for Fcitx5.\n'
printf 'Factory dictionary: %s\n' "$data_root/VanguardFactoryDict4Typing.txtMap"
printf 'Fcitx addon library: %s\n' "$library_dir"
system_addon_dir="$(pkg-config --variable=libdir Fcitx5Core)/fcitx5"
printf 'Add “ZhuyinFlow” in Fcitx5 Configuration → Input Method.\n'
printf 'Restart with: FCITX_ADDON_DIRS=%q fcitx5 --replace -d\n' \
  "$library_dir:$system_addon_dir"
