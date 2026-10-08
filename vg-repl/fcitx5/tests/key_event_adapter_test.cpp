// SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
// SPDX-License-Identifier: MulanPSL-2.0

#include "../src/key_event_adapter.h"

#include <fcitx-utils/testing.h>
#include <cstdint>

int main() {
  fcitx::KeyEvent event(nullptr, fcitx::Key(FcitxKey_s));
  auto converted = zhuyinflow::toBridgeKeyEvent(event);
  FCITX_ASSERT(converted.keycode == FcitxKey_S);
  FCITX_ASSERT(converted.modifiers == 0);
  FCITX_ASSERT(converted.isKeyDown);

  fcitx::KeyEvent shifted(
      nullptr, fcitx::Key(FcitxKey_S, fcitx::KeyStates{fcitx::KeyState::Shift}));
  converted = zhuyinflow::toBridgeKeyEvent(shifted);
  FCITX_ASSERT(converted.keycode == FcitxKey_S);
  FCITX_ASSERT(converted.modifiers == static_cast<uint32_t>(fcitx::KeyState::Shift));

  fcitx::KeyEvent release(nullptr, fcitx::Key(FcitxKey_s), true);
  converted = zhuyinflow::toBridgeKeyEvent(release);
  FCITX_ASSERT(!converted.isKeyDown);

  fcitx::KeyEvent repeated(
      nullptr,
      fcitx::Key(FcitxKey_s, fcitx::KeyStates{fcitx::KeyState::Repeat}));
  converted = zhuyinflow::toBridgeKeyEvent(repeated);
  FCITX_ASSERT(converted.modifiers == static_cast<uint32_t>(fcitx::KeyState::Repeat));
}
