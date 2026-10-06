#ifndef ZHUYINFLOW_FCITX5_KEY_EVENT_ADAPTER_H
#define ZHUYINFLOW_FCITX5_KEY_EVENT_ADAPTER_H

// SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
// SPDX-License-Identifier: MulanPSL-2.0

#include <cstdint>

#include <fcitx/event.h>

namespace zhuyinflow {

struct BridgeKeyEvent {
  uint32_t keycode;
  uint32_t modifiers;
  bool isKeyDown;
};

inline BridgeKeyEvent toBridgeKeyEvent(const fcitx::KeyEvent &event) {
  const auto rawKey = event.rawKey();
  auto keycode = static_cast<uint32_t>(rawKey.sym());
  if (keycode >= static_cast<uint32_t>('a') && keycode <= static_cast<uint32_t>('z')) {
    keycode -= static_cast<uint32_t>('a' - 'A');
  }
  return {
      keycode,
      static_cast<uint32_t>(rawKey.states().toInteger()),
      !event.isRelease(),
  };
}

}  // namespace zhuyinflow

#endif
