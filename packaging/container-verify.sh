#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
# SPDX-License-Identifier: MulanPSL-2.0
set -euo pipefail
[[ $(dpkg --print-architecture) == amd64 ]]
# The official minimal Ubuntu Docker image deliberately strips documentation.
# Retain only this package's docs so strict checksum/license checks exercise
# normal desktop installation contents rather than that image's size policy.
printf 'path-include=/usr/share/doc/zhuyinflow/*\n' > /etc/dpkg/dpkg.cfg.d/zz-zhuyinflow-docs
apt-get update > /verify/apt-update.log 2>&1
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends /package/zhuyinflow.deb binutils desktop-file-utils > /verify/apt-install.log 2>&1
dpkg-query -W -f='${Package} ${Version} ${Status}\n' zhuyinflow > /verify/installed-metadata.txt
dpkg -L zhuyinflow > /verify/installed-files.txt
dpkg --verify zhuyinflow > /verify/dpkg-verify.txt
test ! -s /verify/dpkg-verify.txt
addon=/usr/lib/x86_64-linux-gnu/fcitx5/zhuyinflow.so
private=/usr/lib/x86_64-linux-gnu/zhuyinflow
test -s "$addon"
test -s /usr/share/fcitx5/addon/zhuyinflow.conf
test -s /usr/share/fcitx5/inputmethod/zhuyinflow.conf
test -s /usr/share/zhuyinflow/VanguardFactoryDict4Typing.txtMap
test -s /usr/share/icons/hicolor/index.theme
desktop-file-validate /usr/share/applications/zhuyinflow-settings.desktop
test -x /usr/bin/fcitx5-configtool
ldd "$addon" "$private/"*.so > /verify/installed-ldd.txt
if grep -E 'not found|version .* not found' /verify/installed-ldd.txt; then exit 1; fi
readelf -d "$addon" "$private/"*.so > /verify/installed-elf.txt
if grep -E '(RPATH|RUNPATH).*(/home/|/source/|/build/|/usr/lib/swift)' /verify/installed-elf.txt; then exit 1; fi
grep -F '[$ORIGIN/../zhuyinflow]' /verify/installed-elf.txt
grep -F '[$ORIGIN]' /verify/installed-elf.txt
# Run the installed addon through the real Fcitx Instance/key/commit lifecycle.
export HOME=/verify/home XDG_CONFIG_HOME=/verify/home/.config XDG_DATA_HOME=/verify/home/.local/share
export XDG_CACHE_HOME=/verify/home/.cache XDG_RUNTIME_DIR=/verify/runtime
export FCITX_CONFIG_HOME=/verify/config FCITX_ADDON_DIRS=/usr/lib/x86_64-linux-gnu/fcitx5
export FCITX_DATA_DIRS=/usr/share/fcitx5
export DBUS_SESSION_BUS_ADDRESS=unix:path=/verify/no-bus DISPLAY= WAYLAND_DISPLAY=
unset ZHUYINFLOW_MIXED_ALPHANUMERICAL ZHUYINFLOW_FURIOUS_TYPING_ZHUYIN
mkdir -p "$HOME" "$XDG_CONFIG_HOME" "$XDG_DATA_HOME/zhuyinflow" "$XDG_CACHE_HOME" "$XDG_RUNTIME_DIR" "$FCITX_CONFIG_HOME"
chmod 700 "$XDG_RUNTIME_DIR"
cp /source/vg-repl/fcitx5/tests/profile.in "$FCITX_CONFIG_HOME/profile"
unset ZHUYINFLOW_TEXTMAP
timeout 30 /verify/installed-smoke > /verify/system-map.log 2>&1
grep -F 'INSTALLED_SMOKE_COMMIT_ONCE=你' /verify/system-map.log
grep -F '/usr/share/zhuyinflow/VanguardFactoryDict4Typing.txtMap' /verify/system-map.log
# Personal dictionary wins; then explicit missing override must fail rather than
# hiding a configuration error behind the bundled dictionary.
personal="$XDG_DATA_HOME/zhuyinflow/VanguardFactoryDict4Typing.txtMap"
cp /source/upstream/Packages/vChewing_OSNeutral_LibVanguard/Sources/LXAssemblyMaterials4Tests/Resources/vanguardTextMap_test.txtMap "$personal"
timeout 30 /verify/installed-smoke > /verify/personal-map.log 2>&1
grep -F "$personal" /verify/personal-map.log
ZHUYINFLOW_TEXTMAP=/verify/does-not-exist.txtMap timeout 30 /verify/installed-smoke error > /verify/explicit-missing.log 2>&1
grep -F EXPECTED_DICTIONARY_ERROR /verify/explicit-missing.log
printf 'not a TextMap\n' > "$personal"
timeout 30 /verify/installed-smoke error > /verify/personal-invalid.log 2>&1
grep -F EXPECTED_DICTIONARY_ERROR /verify/personal-invalid.log
# A synthetic HOME verifies opt-in migration without touching real user files.
mkdir -p "$HOME/.local/lib/fcitx5"
printf legacy > "$HOME/.local/lib/fcitx5/zhuyinflow.so"
zhuyinflow-migrate --check > /verify/migration-check.log
test -f "$HOME/.local/lib/fcitx5/zhuyinflow.so"
zhuyinflow-migrate --migrate > /verify/migration.log
test ! -e "$HOME/.local/lib/fcitx5/zhuyinflow.so"
test -s "$personal"
find "$HOME/.local/state/zhuyinflow" -type f > /verify/migration-backup.txt
printf retained > "$XDG_CONFIG_HOME/zhuyinflow-user-preferences"
DEBIAN_FRONTEND=noninteractive apt-get remove -y zhuyinflow > /verify/apt-remove.log 2>&1
test ! -e "$addon"
test ! -e /usr/bin/zhuyinflow-setup
test -s "$personal"
test -s "$XDG_CONFIG_HOME/zhuyinflow-user-preferences"
echo 'PACKAGE_INSTALL_LOADER_DICTIONARY_MIGRATION_REMOVE_OK'
