# Development log

## 2026-10-06 — ZhuyinFlow public-preparation build

- Prepared the Linux/Fcitx 5 remix under the independent name **ZhuyinFlow**,
  with matching Fcitx addon identifiers, descriptors, icon, and configuration
  variables.
- Documented Ubuntu installation, configuration, upgrade from the earlier
  local prototype, removal, upstream attribution, and component-specific
  licenses.
- Added the Fcitx 5 adapter, user-local installer, and TestFrontend smoke
  coverage. Kept the upstream Vanguard submodule unchanged at
  `977a05fe353bcc43cffbc03ea8b6d815a072fd65`.
- Excluded internal planning, evaluation, and handoff files from the release
  snapshot.
- Verified the current source locally with Swift 6.4: 6 tests passed. Built and
  installed the Fcitx addon in a temporary test location; CTest passed 2/2,
  including the TestFrontend smoke test using the pinned factory dictionary.
- Desktop panel rendering and interactive testing in the target Wayland/X11
  session remain to be checked.
