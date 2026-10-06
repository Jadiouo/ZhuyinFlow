// SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
// SPDX-License-Identifier: MulanPSL-2.0

#include "../src/key_event_adapter.h"

#include <cassert>
#include <cstdint>

int main() {
  fcitx::KeyEvent event(nullptr, fcitx::Key(FcitxKey_s));
  auto converted = zhuyinflow::toBridgeKeyEvent(event);
  assert(converted.keycode == FcitxKey_S);
  assert(converted.modifiers == 0);
  assert(converted.isKeyDown);

  fcitx::KeyEvent shifted(
      nullptr, fcitx::Key(FcitxKey_S, fcitx::KeyStates{fcitx::KeyState::Shift}));
  converted = zhuyinflow::toBridgeKeyEvent(shifted);
  assert(converted.keycode == FcitxKey_S);
  assert(converted.modifiers == static_cast<uint32_t>(fcitx::KeyState::Shift));

  fcitx::KeyEvent release(nullptr, fcitx::Key(FcitxKey_s), true);
  converted = zhuyinflow::toBridgeKeyEvent(release);
  assert(!converted.isKeyDown);

  fcitx::KeyEvent repeated(
      nullptr,
      fcitx::Key(FcitxKey_s, fcitx::KeyStates{fcitx::KeyState::Repeat}));
  converted = zhuyinflow::toBridgeKeyEvent(repeated);
  assert(converted.modifiers == static_cast<uint32_t>(fcitx::KeyState::Repeat));
}
