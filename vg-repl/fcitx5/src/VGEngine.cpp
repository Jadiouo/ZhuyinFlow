// SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
// SPDX-License-Identifier: MulanPSL-2.0

#include "key_event_adapter.h"

#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/candidatelist.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/inputmethodentry.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>
#include <fcitx/text.h>
#include <fcitx-utils/log.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "vgbridge.h"

namespace zhuyinflow {
namespace {

using Json = nlohmann::json;

struct BridgeResponse {
  bool handled = false;
  std::string composition;
  int cursor = 0;
  std::vector<std::string> candidates;
  std::string commit;
};

BridgeResponse parseResponse(const char *raw) {
  if (raw == nullptr) {
    throw std::runtime_error("The Swift bridge returned a null response.");
  }
  const auto json = Json::parse(raw);
  if (json.contains("error")) {
    throw std::runtime_error(json.at("error").get<std::string>());
  }
  return {
      json.at("handled").get<bool>(),
      json.at("composition").get<std::string>(),
      json.at("cursor").get<int>(),
      json.at("candidates").get<std::vector<std::string>>(),
      json.at("commit").get<std::string>(),
  };
}

std::optional<std::string> textMapPath() {
  if (const char *configured = std::getenv("ZHUYINFLOW_TEXTMAP")) {
    if (*configured == '\0') {
      return std::nullopt;
    }
    return std::string(configured);
  }

  std::filesystem::path dataDirectory;
  if (const char *xdgDataHome = std::getenv("XDG_DATA_HOME"); xdgDataHome && *xdgDataHome) {
    dataDirectory = xdgDataHome;
  } else if (const char *home = std::getenv("HOME"); home && *home) {
    dataDirectory = std::filesystem::path(home) / ".local" / "share";
  } else {
    return std::nullopt;
  }

  auto path = dataDirectory / "zhuyinflow" / "VanguardFactoryDict4Typing.txtMap";
  if (!std::filesystem::is_regular_file(path)) {
    return std::nullopt;
  }
  return path.string();
}

bool readBooleanEnvironment(const char *name, bool defaultValue) {
  const char *value = std::getenv(name);
  if (value == nullptr) {
    return defaultValue;
  }
  if (std::string(value) == "1") {
    return true;
  }
  if (std::string(value) == "0") {
    return false;
  }
  throw std::invalid_argument(std::string(name) + " must be set to 1 or 0.");
}

std::size_t utf8ByteOffset(const std::string &text, int characterIndex) {
  if (characterIndex <= 0) {
    return 0;
  }

  std::size_t offset = 0;
  int characters = 0;
  while (offset < text.size() && characters < characterIndex) {
    const auto lead = static_cast<unsigned char>(text[offset]);
    if ((lead & 0x80) == 0) {
      offset += 1;
    } else if ((lead & 0xE0) == 0xC0) {
      offset += 2;
    } else if ((lead & 0xF0) == 0xE0) {
      offset += 3;
    } else {
      offset += 4;
    }
    ++characters;
  }
  return std::min(offset, text.size());
}

class VGEngine;

class VGSessionProperty final : public fcitx::InputContextProperty {
 public:
  ~VGSessionProperty() override {
    if (session != nullptr) {
      vg_session_free(session);
    }
  }

  vg_session *session = nullptr;
};

class VGCandidateWord final : public fcitx::CandidateWord {
 public:
  VGCandidateWord(std::string text, VGEngine *engine, int index);
  void select(fcitx::InputContext *inputContext) const override;

 private:
  VGEngine *engine_;
  int index_;
};

class VGEngine final : public fcitx::InputMethodEngineV2 {
 public:
  explicit VGEngine(fcitx::AddonManager *manager)
      : instance_(manager->instance()),
        propertyFactory_([](fcitx::InputContext &) { return new VGSessionProperty(); }),
        mixedAlphanumericalEnabled_(
            readBooleanEnvironment("ZHUYINFLOW_MIXED_ALPHANUMERICAL", true)),
        furiousTypingEnabled4Zhuyin_(
            readBooleanEnvironment("ZHUYINFLOW_FURIOUS_TYPING_ZHUYIN", true)) {
    if (instance_ == nullptr ||
        !instance_->inputContextManager().registerProperty("zhuyinflow-session", &propertyFactory_)) {
      throw std::runtime_error("Could not register ZhuyinFlow input-context state.");
    }
  }

  std::vector<fcitx::InputMethodEntry> listInputMethods() override {
    std::vector<fcitx::InputMethodEntry> entries;
    entries.emplace_back("zhuyinflow", "ZhuyinFlow", "zh_TW", "zhuyinflow");
    entries.back().setNativeName("注音流").setLabel("Z").setIcon("zhuyinflow");
    return entries;
  }

  void activate(const fcitx::InputMethodEntry &, fcitx::InputContextEvent &event) override {
    if (auto *inputContext = event.inputContext()) {
      (void)sessionFor(inputContext);
    }
  }

  void deactivate(const fcitx::InputMethodEntry &, fcitx::InputContextEvent &event) override {
    if (auto *inputContext = event.inputContext()) {
      reset(inputContext);
    }
  }

  void reset(const fcitx::InputMethodEntry &, fcitx::InputContextEvent &event) override {
    if (auto *inputContext = event.inputContext()) {
      reset(inputContext);
    }
  }

  void keyEvent(const fcitx::InputMethodEntry &, fcitx::KeyEvent &event) override {
    auto *inputContext = event.inputContext();
    if (inputContext == nullptr) {
      return;
    }

    try {
      auto *state = sessionFor(inputContext);
      if (state == nullptr || state->session == nullptr) {
        return;
      }
      const auto key = toBridgeKeyEvent(event);
      std::unique_ptr<char, decltype(&vg_string_free)> raw(
          vg_feed_key(state->session, key.keycode, key.modifiers, key.isKeyDown ? 1 : 0),
          &vg_string_free);
      const auto response = parseResponse(raw.get());
      updateUI(inputContext, response);
      if (response.handled) {
        event.filterAndAccept();
      }
    } catch (const std::exception &error) {
      FCITX_ERROR() << "ZhuyinFlow failed to process a key event: " << error.what();
    }
  }

  void selectCandidate(fcitx::InputContext *inputContext, int index) {
    try {
      auto *state = sessionFor(inputContext);
      if (state == nullptr || state->session == nullptr) {
        return;
      }
      std::unique_ptr<char, decltype(&vg_string_free)> raw(
          vg_select_candidate(state->session, index), &vg_string_free);
      updateUI(inputContext, parseResponse(raw.get()));
    } catch (const std::exception &error) {
      FCITX_ERROR() << "ZhuyinFlow failed to select a candidate: " << error.what();
    }
  }

 private:
  VGSessionProperty *sessionFor(fcitx::InputContext *inputContext) {
    auto *state = inputContext->propertyFor(&propertyFactory_);
    if (state == nullptr || state->session != nullptr) {
      return state;
    }

    Json config = {
        {"layout", "dachen"},
        {"mixedAlphanumericalEnabled", mixedAlphanumericalEnabled_},
        {"furiousTypingEnabled4Zhuyin", furiousTypingEnabled4Zhuyin_},
    };
    if (const auto path = textMapPath()) {
      config["lexicon"] = *path;
    }
    const auto serialized = config.dump();
    state->session = vg_session_new(serialized.c_str());
    if (state->session == nullptr) {
      throw std::runtime_error("Could not create the Swift input session; check the TextMap.");
    }
    return state;
  }

  void reset(fcitx::InputContext *inputContext) {
    auto *state = sessionFor(inputContext);
    if (state == nullptr || state->session == nullptr) {
      return;
    }
    vg_reset(state->session);
    updateUI(inputContext, {});
  }

  void updateUI(fcitx::InputContext *inputContext, const BridgeResponse &response) {
    auto &panel = inputContext->inputPanel();
    fcitx::Text preedit(response.composition);
    preedit.setCursor(static_cast<int>(utf8ByteOffset(response.composition, response.cursor)));
    panel.setPreedit(preedit);

    if (response.candidates.empty()) {
      panel.setCandidateList(nullptr);
    } else {
      auto candidates = std::make_unique<fcitx::CommonCandidateList>();
      candidates->setPageSize(9);
      for (std::size_t index = 0; index < response.candidates.size(); ++index) {
        candidates->append<VGCandidateWord>(
            response.candidates[index], this, static_cast<int>(index));
      }
      panel.setCandidateList(std::move(candidates));
    }

    inputContext->updatePreedit();
    inputContext->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    if (!response.commit.empty()) {
      inputContext->commitString(response.commit);
    }
  }

  fcitx::Instance *instance_;
  fcitx::FactoryFor<VGSessionProperty> propertyFactory_;
  bool mixedAlphanumericalEnabled_;
  bool furiousTypingEnabled4Zhuyin_;
};

VGCandidateWord::VGCandidateWord(std::string text, VGEngine *engine, int index)
    : CandidateWord(fcitx::Text(std::move(text))), engine_(engine), index_(index) {}

void VGCandidateWord::select(fcitx::InputContext *inputContext) const {
  engine_->selectCandidate(inputContext, index_);
}

class VGEngineFactory final : public fcitx::AddonFactory {
 public:
  fcitx::AddonInstance *create(fcitx::AddonManager *manager) override {
    return new VGEngine(manager);
  }
};

}  // namespace
}  // namespace zhuyinflow

FCITX_ADDON_FACTORY(zhuyinflow::VGEngineFactory)
