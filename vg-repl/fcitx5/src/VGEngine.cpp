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
  bool candidateSelectionActive = false;
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
      json.value("candidateSelectionActive", false),
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
  }

  if (!dataDirectory.empty()) {
    auto path = dataDirectory / "zhuyinflow" / "VanguardFactoryDict4Typing.txtMap";
    if (std::filesystem::is_regular_file(path)) {
      return path.string();
    }
  }
#ifdef VG_SYSTEM_TEXTMAP
  // A Debian package supplies the factory dictionary without modifying HOME.
  // Explicit environment overrides above remain authoritative, even if bad.
  if (std::filesystem::is_regular_file(VG_SYSTEM_TEXTMAP)) {
    return std::string(VG_SYSTEM_TEXTMAP);
  }
#endif
  return std::nullopt;
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
  BridgeResponse snapshot;
  int candidatePage = 0;
  int candidateCursor = 0;
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
            readBooleanEnvironment("ZHUYINFLOW_FURIOUS_TYPING_ZHUYIN", false)) {
    if (instance_ == nullptr ||
        !instance_->inputContextManager().registerProperty("zhuyinflow-session", &propertyFactory_)) {
      throw std::runtime_error("Could not register ZhuyinFlow input-context state.");
    }
    capabilityWatcher_ = instance_->watchEvent(
        fcitx::EventType::InputContextCapabilityChanged,
        fcitx::EventWatcherPhase::InputMethod, [this](fcitx::Event &event) {
          auto *inputContext = static_cast<fcitx::InputContextEvent &>(event).inputContext();
          if (inputContext && inputContext->hasFocus() &&
              instance_->inputMethod(inputContext) == "zhuyinflow") {
            activateContext(inputContext);
          }
        });
  }

  std::vector<fcitx::InputMethodEntry> listInputMethods() override {
    std::vector<fcitx::InputMethodEntry> entries;
    entries.emplace_back("zhuyinflow", "ZhuyinFlow", "zh_TW", "zhuyinflow");
    entries.back().setNativeName("注音流").setLabel("Z").setIcon("zhuyinflow");
    return entries;
  }

  void activate(const fcitx::InputMethodEntry &, fcitx::InputContextEvent &event) override {
    if (auto *inputContext = event.inputContext()) {
      activateContext(inputContext);
    }
  }

  void deactivate(const fcitx::InputMethodEntry &, fcitx::InputContextEvent &event) override {
    if (auto *inputContext = event.inputContext()) {
      // Focus loss and IM switching suspend this context, not its composition.
      rememberCandidatePosition(inputContext);
      inputContext->inputPanel().reset();
      inputContext->updatePreedit();
      inputContext->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
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
      if (!event.isRelease() && handleCandidateKey(inputContext, event)) {
        event.filterAndAccept();
        return;
      }
      const auto key = toBridgeKeyEvent(event);
      std::unique_ptr<char, decltype(&vg_string_free)> raw(
          vg_feed_key(state->session, key.keycode, key.modifiers, key.isKeyDown ? 1 : 0),
          &vg_string_free);
      const auto response = parseResponse(raw.get());
      // Key releases and unhandled keys must not reconstruct candidates.
      if (!event.isRelease()) {
        updateUI(inputContext, response);
      }
      if (response.handled) {
        event.filterAndAccept();
      }
    } catch (const std::exception &error) {
      showError(inputContext, "process a key event", error);
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
      showError(inputContext, "select a candidate", error);
    }
  }

 private:
  void rememberCandidatePosition(fcitx::InputContext *inputContext) {
    auto *state = inputContext->propertyFor(&propertyFactory_);
    auto list = inputContext->inputPanel().candidateList();
    if (auto *candidates = dynamic_cast<fcitx::CommonCandidateList *>(list.get());
        state && candidates) {
      state->candidatePage = candidates->currentPage();
      state->candidateCursor = candidates->globalCursorIndex();
    }
  }

  void activateContext(fcitx::InputContext *inputContext) {
    try {
      if (auto *state = sessionFor(inputContext); state && state->session) {
        updateUI(inputContext, state->snapshot);
      }
    } catch (const std::exception &error) {
      showError(inputContext, "activate the input method", error);
    }
  }

  void showError(fcitx::InputContext *inputContext, const char *operation,
                 const std::exception &error) {
    FCITX_ERROR() << "ZhuyinFlow failed to " << operation << ": " << error.what();
    auto &panel = inputContext->inputPanel();
    panel.reset();
    // Fcitx uses auxUp for transient input-method switch information.
    panel.setAuxDown(fcitx::Text(std::string("ZhuyinFlow unavailable: check TextMap. ") + error.what()));
    inputContext->updatePreedit();
    inputContext->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  }

  bool handleCandidateKey(fcitx::InputContext *inputContext, fcitx::KeyEvent &event) {
    auto *state = inputContext->propertyFor(&propertyFactory_);
    if (!state || !state->snapshot.candidateSelectionActive) {
      return false;
    }
    auto list = inputContext->inputPanel().candidateList();
    auto *candidates = dynamic_cast<fcitx::CommonCandidateList *>(list.get());
    if (!candidates || candidates->empty() ||
        event.rawKey().states().testAny(fcitx::KeyStates{
            fcitx::KeyState::Ctrl, fcitx::KeyState::Alt,
            fcitx::KeyState::Super, fcitx::KeyState::Shift})) {
      return false;
    }
    const auto sym = event.key().sym();
    if (sym >= FcitxKey_1 && sym <= FcitxKey_9) {
      const int localIndex = sym - FcitxKey_1;
      if (localIndex < candidates->size()) {
        candidates->candidate(localIndex).select(inputContext);
      }
    } else if (sym == FcitxKey_Down || sym == FcitxKey_Right) {
      candidates->nextCandidate();
    } else if (sym == FcitxKey_Up || sym == FcitxKey_Left) {
      candidates->prevCandidate();
    } else if (sym == FcitxKey_Page_Down || sym == FcitxKey_space) {
      candidates->next();
    } else if (sym == FcitxKey_Page_Up) {
      candidates->prev();
    } else if (sym == FcitxKey_Escape) {
      // The upstream candidate controller is absent on Linux. Cancel here so
      // Escape cannot be swallowed while leaving the composition intact.
      reset(inputContext);
    } else if (sym == FcitxKey_Return) {
      // Selection updates the bridge's composition; let the original Return
      // then follow its normal commit path.
      candidates->candidate(candidates->cursorIndex()).select(inputContext);
      return false;
    } else {
      return false;
    }
    inputContext->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    return true;
  }

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
    // Clearing an absent state must not attempt dictionary/session creation.
    try {
      auto *state = inputContext->propertyFor(&propertyFactory_);
      if (state && state->session) {
        vg_reset(state->session);
      }
      inputContext->inputPanel().reset();
      updateUI(inputContext, {});
    } catch (const std::exception &error) {
      showError(inputContext, "reset the input method", error);
    }
  }

  void updateUI(fcitx::InputContext *inputContext, const BridgeResponse &response) {
    auto &panel = inputContext->inputPanel();
    auto *state = inputContext->propertyFor(&propertyFactory_);
    fcitx::Text preedit;
    if (!response.composition.empty()) {
      preedit.append(response.composition);
    }
    preedit.setCursor(static_cast<int>(utf8ByteOffset(response.composition, response.cursor)));
    panel.setPreedit(preedit);
    // Retained composition is not a focus-out commit. Clients that autonomously
    // commit on unfocus may ignore DontCommit, so use panel-only preedit for them.
    fcitx::Text clientPreedit;
    if (!response.composition.empty() &&
        inputContext->capabilityFlags().test(fcitx::CapabilityFlag::Preedit) &&
        !inputContext->capabilityFlags().test(fcitx::CapabilityFlag::ClientUnfocusCommit)) {
      clientPreedit = fcitx::Text(response.composition, fcitx::TextFormatFlag::DontCommit);
      clientPreedit.setCursor(preedit.cursor());
    }
    panel.setClientPreedit(clientPreedit);
    panel.setAuxUp({});
    panel.setAuxDown({});

    if (response.candidates.empty()) {
      panel.setCandidateList(nullptr);
    } else if (!state || state->snapshot.candidates != response.candidates ||
               !panel.candidateList()) {
      auto candidates = std::make_unique<fcitx::CommonCandidateList>();
      candidates->setPageSize(9);
      candidates->setSelectionKey({fcitx::Key(FcitxKey_1), fcitx::Key(FcitxKey_2),
                                  fcitx::Key(FcitxKey_3), fcitx::Key(FcitxKey_4),
                                  fcitx::Key(FcitxKey_5), fcitx::Key(FcitxKey_6),
                                  fcitx::Key(FcitxKey_7), fcitx::Key(FcitxKey_8),
                                  fcitx::Key(FcitxKey_9)});
      candidates->setCursorPositionAfterPaging(fcitx::CursorPositionAfterPaging::ResetToFirst);
      for (std::size_t index = 0; index < response.candidates.size(); ++index) {
        candidates->append<VGCandidateWord>(
            response.candidates[index], this, static_cast<int>(index));
      }
      const bool restoring = state && state->snapshot.candidates == response.candidates;
      candidates->setPage(restoring ? state->candidatePage : 0);
      candidates->setGlobalCursorIndex(restoring ? state->candidateCursor : 0);
      panel.setCandidateList(std::move(candidates));
    }
    if (state) {
      state->snapshot = response;
      // A UI restoration must never replay an already delivered commit.
      state->snapshot.commit.clear();
    }

    inputContext->updatePreedit();
    inputContext->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    if (!response.commit.empty()) {
      inputContext->commitString(response.commit);
    }
  }

  fcitx::Instance *instance_;
  fcitx::FactoryFor<VGSessionProperty> propertyFactory_;
  std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>> capabilityWatcher_;
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
