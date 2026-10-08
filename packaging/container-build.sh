#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
# SPDX-License-Identifier: MulanPSL-2.0
set -euo pipefail
[[ $(dpkg --print-architecture) == amd64 ]]
version=${ZHUYINFLOW_VERSION:?}
multiarch=$(dpkg-architecture -qDEB_HOST_MULTIARCH)
private=/usr/lib/$multiarch/zhuyinflow
root=/build/package-root
mkdir -p /build/lexicon /build/swift "$root"
# SwiftPM plugins can write inside their package; build a clean writable export.
cp -a /source/lexicon-source/. /build/lexicon/
(cd /build/lexicon && VANGUARD_OUTPUT_DIR=/build/dictionary swift run -c release VCDataBuilder vanguardTextMap) > /build/dictionary-build.log 2>&1
dictionary=/build/dictionary/Build/Release/vanguard-textmap/VanguardFactoryDict4Typing.txtMap
test -s "$dictionary"
swift build --package-path /source/vg-repl --scratch-path /build/swift -c release --product vgbridge > /build/swift-build.log 2>&1
bin=$(swift build --package-path /source/vg-repl --scratch-path /build/swift -c release --show-bin-path)
cmake -S /source/vg-repl/fcitx5 -B /build/fcitx -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr -DVG_BRIDGE_LIBRARY="$bin/libvgbridge.so" \
  -DVG_VANGUARD_LIBRARY="$bin/libVanguard.so" \
  -DVG_INSTALL_ADDON_DIR="/usr/lib/$multiarch/fcitx5" -DVG_INSTALL_RUNTIME_DIR="$private" \
  '-DVG_INSTALL_RPATH=$ORIGIN/../zhuyinflow' \
  -DVG_SYSTEM_TEXTMAP=/usr/share/zhuyinflow/VanguardFactoryDict4Typing.txtMap > /build/cmake-configure.log 2>&1
cmake --build /build/fcitx --parallel 2 > /build/cmake-build.log 2>&1
LD_LIBRARY_PATH="$bin:/usr/lib/swift/linux" ctest --test-dir /build/fcitx --output-on-failure > /build/ctest.log 2>&1
DESTDIR="$root" cmake --install /build/fcitx > /build/cmake-install.log 2>&1
# Bundle only the Swift image's runtime closure; Ubuntu owns libc and Fcitx.
ldd "$bin/libvgbridge.so" "$bin/libVanguard.so" > /build/runtime-closure.txt
awk '$3 ~ /^\/usr\/lib\/swift\/linux\// { print $3 }' /build/runtime-closure.txt | sort -u > /build/runtime-files.txt
while IFS= read -r runtime; do cp -L "$runtime" "$root$private/"; done < /build/runtime-files.txt
for library in "$root$private/"*.so; do patchelf --set-rpath '$ORIGIN' "$library"; done
install -Dm644 "$dictionary" "$root/usr/share/zhuyinflow/VanguardFactoryDict4Typing.txtMap"
install -Dm755 /source/packaging/zhuyinflow-setup "$root/usr/bin/zhuyinflow-setup"
install -Dm755 /source/packaging/zhuyinflow-migrate "$root/usr/bin/zhuyinflow-migrate"
install -Dm644 /source/packaging/zhuyinflow-settings.desktop "$root/usr/share/applications/zhuyinflow-settings.desktop"
desktop-file-validate "$root/usr/share/applications/zhuyinflow-settings.desktop"
doc="$root/usr/share/doc/zhuyinflow"
mkdir -p "$doc/copyrights"
cp /source/LICENSE "$doc/copyright"
cp -a /source/LICENSES "$doc/copyrights/project"
cp -a /source/packaging/licenses/. "$doc/copyrights/redistributed/"
cp /source/lexicon-source/LICENSE.txt "$doc/copyrights/redistributed/VanguardLexicon-LICENSE.txt"
cp /source/lexicon-source/README.MD "$doc/copyrights/redistributed/VanguardLexicon-README.md"
cp /source/packaging/README.md "$doc/README.md"
cp /source/BUILD-PROVENANCE "$doc/BUILD-PROVENANCE"
cp /source/SWIFT-IMAGE /source/BUILD-PACKAGES "$doc/"
cp /source/SOURCE-SHA256 "$doc/SOURCE-SHA256"
printf 'zhuyinflow (%s) noble; urgency=medium\n\n  * Initial Ubuntu 24.04 amd64 package.\n\n -- ZhuyinFlow contributors <noreply@zhuyinflow.invalid>  %s\n' "$version" "$(date -u -d "@$SOURCE_DATE_EPOCH" -R)" | gzip -n > "$doc/changelog.Debian.gz"
mkdir -p /build/debian "$root/DEBIAN"
cat > /build/debian/control <<EOF
Source: zhuyinflow
Section: utils
Priority: optional
Maintainer: ZhuyinFlow contributors <noreply@zhuyinflow.invalid>
Package: zhuyinflow
Architecture: amd64
Description: Traditional Chinese Zhuyin input method for Fcitx 5
EOF
# Only private unversioned Swift libraries lack Debian shlibs metadata. Host
# libraries are resolved against installed Ubuntu packages by dpkg-shlibdeps.
(cd /build && dpkg-shlibdeps --ignore-missing-info -O -l"$root$private" \
  -e"$root/usr/lib/$multiarch/fcitx5/zhuyinflow.so" \
  $(find "$root$private" -name '*.so' -printf '-e%p ')) > /build/shlibs.out 2> /build/shlibs.log
dependencies=$(sed -n 's/^shlibs:Depends=//p' /build/shlibs.out)
test -n "$dependencies"
cat > "$root/DEBIAN/control" <<EOF
Package: zhuyinflow
Version: $version
Architecture: amd64
Section: utils
Priority: optional
Maintainer: ZhuyinFlow contributors <noreply@zhuyinflow.invalid>
Depends: fcitx5 (>= 5.1.7), fcitx5-config-qt, hicolor-icon-theme, $dependencies
Installed-Size: $(du -sk "$root/usr" | cut -f1)
Description: Traditional Chinese Zhuyin input method for Fcitx 5
 ZhuyinFlow provides Dachen Zhuyin composition and candidate selection,
 a factory dictionary and a settings entry for Ubuntu 24.04 amd64.
 Built on the pinned Vanguard core; independent of the vChewing project.
EOF
find "$root" -type d -exec chmod 755 {} +
find "$root" -type f -exec touch -d "@$SOURCE_DATE_EPOCH" {} +
(cd "$root" && find usr -type f -print0 | sort -z | xargs -0 md5sum) > "$root/DEBIAN/md5sums"
dpkg-deb --root-owner-group --build "$root" "/out/zhuyinflow_${version}_amd64.deb"
cat /build/ctest.log
