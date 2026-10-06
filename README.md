# ZhuyinFlow

ZhuyinFlow is an independent Linux/Fcitx 5 input method built for personal use
from the open-source vChewing input engine. It combines the upstream Swift
typing core with a small Swift C-ABI bridge and a native Fcitx 5 C++ adapter.
The project is not an official vChewing release and is not endorsed by its
upstream maintainers.

The name and icon are independent of vChewing. The icon uses the first letter
of **ZhuyinFlow**.

## Features

- Traditional Zhuyin input using the Dachen keyboard layout.
- Chinese/English mixed typing.
- Zhuyin furious typing. With mixed typing enabled, ASCII input is handled by
  the mixed-typing path and automatic syllable chopping is suppressed by the
  upstream engine; disable mixed typing to use automatic chopping.
- Per-Fcitx-input-context composition, candidate selection, and commits.
- Swift 6.4 bridge and Fcitx 5 native addon.

The input method is configured to enable mixed typing and Zhuyin furious
typing by default. See [Configuration](#configuration) to change either.

## Install on Ubuntu

The supported setup currently targets Ubuntu 24.04 and Fcitx 5. Install the
Fcitx 5 development files, CMake, pkg-config, Git, Docker, and a working Docker
daemon first. The installer uses the `swift:6.4-noble` container image to build
the Swift components; Swift does not need to be installed on the host.

Clone this repository with its pinned upstream submodule:

```sh
git clone --recurse-submodules https://github.com/Jadiouo/ZhuyinFlow.git
cd ZhuyinFlow
./vg-repl/fcitx5/install.sh
```

The installer downloads the pinned Vanguard Lexicon source, builds the factory
TextMap locally, compiles the bridge and Fcitx addon, runs the test suite, and
installs the result under user-local directories. It does not require root
privileges, change the system default input method, or restart Fcitx.

Add **ZhuyinFlow** in Fcitx Configuration → Input Method, then restart Fcitx
using the command printed by the installer. If your desktop does not inherit
the installer's addon path, use the printed `FCITX_ADDON_DIRS` value when
restarting Fcitx.

### Upgrade from the earlier local prototype

Earlier local builds of this project used the Fcitx ID `vchewing`. After
confirming ZhuyinFlow works, remove that old entry from your Fcitx profile. If
those old files came only from this project's installer, remove its obsolete
plugin and descriptors; do not use this cleanup for a separately installed
vChewing add-on:

```sh
rm -f "$HOME/.local/lib/fcitx5/vchewing.so" \
  "$HOME/.local/share/fcitx5/addon/vchewing.conf" \
  "$HOME/.local/share/fcitx5/inputmethod/vchewing.conf"
```

## Use

Select **ZhuyinFlow** in Fcitx and type with the Dachen Zhuyin keyboard. For
example, `s u 3` produces the reading ㄋㄧˇ; choose a candidate or confirm it
with Space/Enter according to the active Fcitx key bindings.

The project also includes a spelling-only REPL that does not need the factory
dictionary:

```sh
./vg-repl/run.sh --layout dachen
```

Enter one key per line (`s`, `u`, `3`, `backspace`, `space`, or `shift+a`).
Each line of output is a JSON response. End input with Ctrl+D.

## Configuration

Set these environment variables for the Fcitx process; changes take effect
after restarting Fcitx:

| Variable | Default | Meaning |
|---|---:|---|
| `ZHUYINFLOW_MIXED_ALPHANUMERICAL` | `1` | Enable Chinese/English mixed typing. Set to `0` to disable it. |
| `ZHUYINFLOW_FURIOUS_TYPING_ZHUYIN` | `1` | Enable Zhuyin furious typing. Set to `0` to disable it. |
| `ZHUYINFLOW_TEXTMAP` | installed TextMap | Use a specific factory TextMap path. |
| `ZHUYINFLOW_REBUILD_TEXTMAP` | `0` | Set to `1` to rebuild the factory TextMap during installation. |
| `ZHUYINFLOW_SWIFT_IMAGE` | `swift:6.4-noble` | Override the Swift Docker image used by the installer. |

When mixed typing is enabled, the upstream core continues to use furious-typing
reading suggestions, but does not automatically chop sequential Zhuyin
syllables. Set `ZHUYINFLOW_MIXED_ALPHANUMERICAL=0` and leave furious typing
enabled if automatic syllable chopping is preferred.

## Build and test

Build the Swift bridge with Swift 6.4:

```sh
docker run --rm -v "$PWD":/work -w /work swift:6.4-noble \
  swift build --package-path /work/vg-repl --product vgbridge
```

Run the Fcitx adapter and TestFrontend integration tests through the installer,
or build the Fcitx addon with CMake after building the bridge. The installer
also checks shared-library dependencies before reporting success.

## Uninstall

Remove **ZhuyinFlow** from Fcitx Configuration, then remove the user-local
files installed by this project:

```sh
rm -f "$HOME/.local/lib/fcitx5/zhuyinflow.so" \
  "$HOME/.local/lib/fcitx5/libvgbridge.so" \
  "$HOME/.local/lib/fcitx5/libVanguard.so"
rm -f "$HOME/.local/share/fcitx5/addon/zhuyinflow.conf" \
  "$HOME/.local/share/fcitx5/inputmethod/zhuyinflow.conf" \
  "$HOME/.local/share/icons/hicolor/scalable/apps/zhuyinflow.svg"
rm -rf "${XDG_DATA_HOME:-$HOME/.local/share}/zhuyinflow"
```

Restart Fcitx after uninstalling. This does not remove Fcitx itself, system
packages, or any other input method. The installer also copies Swift runtime
libraries into the Fcitx library directory; inspect their dependencies before
manually removing any, as another local program may share them.

## Upstream and licensing

ZhuyinFlow is an independent remix, not a replacement or official port. Its
upstream source is the [vChewing macOS repository](https://github.com/vChewing/vChewing-macOS),
pinned as a submodule at commit
`977a05fe353bcc43cffbc03ea8b6d815a072fd65`. The factory lexicon is built from
the separate [Vanguard Lexicon repository](https://github.com/vChewing/vChewing-VanguardLexicon),
pinned by the installer at commit
`d41f2fc244eadf94c37df50ef98e716fdc28146d`.

ZhuyinFlow-authored project files are licensed under MulanPSL-2.0. The upstream
Vanguard modules retain their LGPLv3-or-later licenses and module-specific
custom exceptions; those terms do not change the license of this project’s own
files. See [`LICENSE`](LICENSE) and
[`LICENSES/README.md`](LICENSES/README.md) before redistributing source or
binaries. This repository includes upstream notices and license texts.

The vChewing name, logos, and marks are not granted for branding this remix.
“ZhuyinFlow” is an independent project name. The upstream project asks people
to discuss proposed contributions before sending them; we have not submitted
this work upstream.

## Status and limitations

- Tested locally with Swift 6.4, Fcitx 5.1.7, Ubuntu 24.04, and Fcitx
  TestFrontend. The current source passed 6 Swift tests and 2 Fcitx CTest
  tests in the local build.
- Desktop application behavior and panel-specific icon rendering should still
  be checked in the target Wayland/X11 session.
- LeakSanitizer and desktop-session testing have not been run for the current
  renamed source tree.
- This project is provided as-is, without warranty.

See [`docs/desktop-testing.md`](docs/desktop-testing.md) for the manual desktop
checklist and [`vg-repl/README.md`](vg-repl/README.md) for bridge details.
