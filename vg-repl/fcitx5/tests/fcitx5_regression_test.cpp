// SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
// SPDX-License-Identifier: MulanPSL-2.0

#include <fcitx-utils/capabilityflags.h>
#include <fcitx-utils/eventdispatcher.h>
#include <fcitx-utils/key.h>
#include <fcitx/addonmanager.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addonloader.h>
#include <fcitx/candidatelist.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/inputmethodmanager.h>
#include <fcitx/inputmethodentry.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>
#include <fcitx/surroundingtext.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace fcitx;

// The packaged keyboard engine is linked into the fcitx5 executable, not the
// Core shared library. Register a minimal pass-through IM in this test process
// to exercise actual framework switching without a desktop daemon.
class PassThroughKeyboard final : public InputMethodEngineV2 {
 public:
  std::vector<InputMethodEntry> listInputMethods() override {
    std::vector<InputMethodEntry> entries;
    entries.emplace_back("keyboard-us", "Test US keyboard", "en", "keyboard");
    return entries;
  }
  void keyEvent(const InputMethodEntry &, KeyEvent &) override {}
};

class PassThroughKeyboardFactory final : public AddonFactory {
 public:
  AddonInstance *create(AddonManager *) override { return new PassThroughKeyboard(); }
};

static void check(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

// This frontend goes through Instance's event dispatch, including the framework's
// ReservedFirst focus-out commit, rather than calling VGEngine in isolation.
class TestContext final : public InputContext {
 public:
  TestContext(InputContextManager &manager, CapabilityFlags capabilities)
      : InputContext(manager, "zhuyinflow-regression") {
    setCapabilityFlags(capabilities);
    created();
  }
  ~TestContext() override { destroy(); }
  const char *frontend() const override { return "zhuyinflow-regression"; }
  std::vector<std::string> commits;
  std::string visiblePreedit;
  std::string clientCommitPreedit;
  void loseFocus() {
    // A frontend with ClientUnfocusCommit is responsible for this local commit.
    if (capabilityFlags().test(CapabilityFlag::ClientUnfocusCommit) &&
        !clientCommitPreedit.empty()) {
      commits.push_back(clientCommitPreedit);
    }
    focusOut();
  }

 protected:
  void commitStringImpl(const std::string &text) override { commits.push_back(text); }
  void deleteSurroundingTextImpl(int, unsigned int) override {}
  void forwardKeyImpl(const ForwardKeyEvent &) override {}
  void updatePreeditImpl() override {
    visiblePreedit = inputPanel().clientPreedit().toString();
    // Unformatted clients cannot honor DontCommit; emulate that responsibility.
    clientCommitPreedit = capabilityFlags().test(CapabilityFlag::FormattedPreedit)
                              ? inputPanel().clientPreedit().toStringForCommit()
                              : visiblePreedit;
  }
};

static bool key(TestContext &context, const std::string &name, bool release = false) {
  KeyEvent event(&context, Key(name), release);
  return context.keyEvent(event);
}

static void typeReading(TestContext &context) {
  for (const auto *name : {"s", "u", "3"}) {
    key(context, name);
    key(context, name, true);
  }
  check(context.inputPanel().preedit().toString() == "你", "su3 produces 你");
}

static void candidateRegression(TestContext &context) {
  typeReading(context);
  check(key(context, "Down"), "Down opens explicit candidate selection");
  auto list = context.inputPanel().candidateList();
  check(list && list->toBulk()->totalSize() > 9, "reading has multiple candidate pages");
  check(list->label(0).toString() == "1. ", "number label matches selection key");
  check(list->cursorIndex() == 0, "initial candidate is highlighted");
  check(key(context, "Down"), "Down is handled");
  check(list->cursorIndex() == 1, "Down moves the candidate cursor");
  key(context, "Down", true);
  check(context.inputPanel().candidateList() == list && list->cursorIndex() == 1,
        "Down release retains the list and cursor");
  key(context, "Up");
  check(list->cursorIndex() == 0, "Up moves back");
  key(context, "Right");
  check(list->cursorIndex() == 1, "Right moves forward");
  key(context, "Left");
  check(list->cursorIndex() == 0, "Left moves back");
  key(context, "Page_Down");
  check(list->toPageable()->currentPage() == 1 && list->cursorIndex() == 0,
        "PageDown moves to page two and highlights its first candidate");
  key(context, "Page_Down", true);
  check(context.inputPanel().candidateList() == list && list->toPageable()->currentPage() == 1,
        "PageDown release preserves page two");
  key(context, "Page_Up");
  check(list->toPageable()->currentPage() == 0, "PageUp moves back");
  key(context, "space");
  check(list->toPageable()->currentPage() == 1 && list->cursorIndex() == 0,
        "candidate Space pages forward and highlights the first candidate");
  for (int repeat = 0; repeat < 3; ++repeat) {
    key(context, "space", true);
    key(context, "space");
    check(context.inputPanel().candidateList() == list &&
              list->toPageable()->currentPage() == 1 && list->cursorIndex() == 0 &&
              context.commits.empty(),
          "repeated candidate Space is stable at the last page and never commits");
  }
  key(context, "Page_Up");
  list->toPageable()->next();
  key(context, "Page_Down", true);
  check(context.inputPanel().candidateList() == list && list->toPageable()->currentPage() == 1,
        "API paging survives a subsequent release");
  const auto selected = list->candidate(1).text().toString();
  key(context, "2");
  check(context.inputPanel().preedit().toString() == selected, "2 selects page-local candidate two");
  key(context, "Return");
  check(context.commits == std::vector<std::string>{selected}, "selected candidate is committed exactly once");
  key(context, "Return", true);
  check(context.commits.size() == 1, "Return release does not duplicate the commit");
  typeReading(context);
  key(context, "Down");
  list = context.inputPanel().candidateList();
  key(context, "Down");
  const auto highlighted = list->candidate(list->cursorIndex()).text().toString();
  key(context, "Return");
  check(context.commits == std::vector<std::string>{selected, highlighted},
        "Return commits the highlighted candidate");
  typeReading(context);
  key(context, "Down");
  key(context, "Page_Down");
  key(context, "Down");
  list = context.inputPanel().candidateList();
  check(list->toPageable()->currentPage() == 1 && list->cursorIndex() == 1,
        "candidate Escape fixture uses page two and a nonzero cursor");
  const auto committedBeforeCancel = context.commits;
  for (const auto *modifiedEscape : {"Control+Escape", "Alt+Escape", "Shift+Escape", "Super+Escape"}) {
    key(context, modifiedEscape);
    check(context.inputPanel().preedit().toString() == "你" &&
              context.inputPanel().candidateList() == list &&
              context.commits == committedBeforeCancel,
          "modified Escape does not invoke unconditional candidate cancellation");
  }
  check(key(context, "Escape"), "candidate Escape is handled");
  key(context, "Escape", true);
  check(context.inputPanel().empty() && context.visiblePreedit.empty() &&
            context.commits == committedBeforeCancel,
        "candidate Escape clears all UI and composition without committing");
  typeReading(context);
  key(context, "Return");
  check(context.commits.size() == committedBeforeCancel.size() + 1 &&
            context.commits.back() == "你",
        "ordinary composition remains usable after candidate cancellation");
}

static void continuousInputRegression(TestContext &context) {
  for (const auto *initial : {"2", "5", "8"}) {
    context.reset();
    typeReading(context);
    key(context, initial);
    check(context.inputPanel().preedit().toString().size() > std::string("你").size() &&
              context.inputPanel().preedit().toString().find("你") == 0,
          std::string("Dachen ") + initial + " begins a new syllable instead of selecting a candidate");
  }
  context.reset();
  for (const auto *name : {"f", "i", "l", "m", "2", "0", "2", "6"}) {
    key(context, name);
    key(context, name, true);
  }
  check(context.commits.empty(), "mixed ASCII digits are not prematurely selected/committed");
  key(context, "space");
  check(context.commits == std::vector<std::string>{"film2026 "},
        "mixed ASCII text with digits commits unchanged");
}

static void focusRegression(Instance &instance, TestContext &context) {
  typeReading(context);
  const auto originalFlags = context.capabilityFlags();
  const bool inlineExpected = originalFlags.test(CapabilityFlag::Preedit) &&
                              !originalFlags.test(CapabilityFlag::ClientUnfocusCommit);
  check(context.visiblePreedit == (inlineExpected ? "你" : ""),
        "capabilities choose safe inline or panel-only preedit");
  check(context.inputPanel().clientPreedit().toStringForCommit().empty(),
        "all retained inline text is marked DontCommit");
  context.setCapabilityFlags(CapabilityFlags{CapabilityFlag::Preedit, CapabilityFlag::ClientUnfocusCommit});
  check(context.visiblePreedit.empty() && context.inputPanel().clientPreedit().empty(),
        "capability change clears previously delivered inline preedit");
  context.setCapabilityFlags(originalFlags);
  check(context.visiblePreedit == (inlineExpected ? "你" : ""),
        "capability change restores only safe inline preedit");
  auto list = context.inputPanel().candidateList();
  list->toPageable()->next();
  list->toCursorMovable()->nextCandidate();
  check(list->cursorIndex() == 1, "focus fixture has a nonzero page-local cursor");
  context.loseFocus();
  check(context.commits.empty(), "focus-out must not commit retained composition");
  check(context.inputPanel().empty(), "focus-out hides the input panel");
  {
    TestContext other(instance.inputContextManager(), originalFlags);
    other.focusIn();
    instance.setCurrentInputMethod(&other, "zhuyinflow", false);
    check(other.inputPanel().preedit().empty(), "a second input context does not inherit composition");
    key(other, "e");
    check(!other.inputPanel().preedit().empty(), "a second input context has its own reading");
    other.loseFocus();
  }
  context.focusIn();
  check(context.inputPanel().preedit().toString() == "你", "focus-in restores completed composition");
  check(context.inputPanel().candidateList()->toPageable()->currentPage() == 1 &&
            context.inputPanel().candidateList()->cursorIndex() == list->cursorIndex(),
        "focus-in preserves the candidate page");
  key(context, "Return");
  check(context.commits == std::vector<std::string>{"你"}, "restored composition commits once");
  key(context, "e");
  key(context, "l");
  const auto reading = context.inputPanel().preedit().toString();
  check(!reading.empty(), "unfinished reading is visible");
  context.loseFocus();
  context.focusIn();
  check(context.inputPanel().preedit().toString() == reading, "unfinished reading survives focus-out");
  check(context.commits.size() == 1, "raw reading is not committed on focus-out");
  context.reset();
  check(context.inputPanel().preedit().empty(), "explicit reset cancels reading");
  typeReading(context);
  instance.setCurrentInputMethod(&context, "keyboard-us", false);
  check(instance.inputMethod(&context) == "keyboard-us", "keyboard input method is actually active");
  check(context.commits.size() == 1 && context.visiblePreedit.empty(),
        "switch hides retained composition without a duplicate commit");
  check(!key(context, "a"), "ordinary key returns to keyboard client");
  key(context, "a", true);
  context.surroundingText().setText("a", 1, 1);
  context.updateSurroundingText();
  instance.setCurrentInputMethod(&context, "zhuyinflow", false);
  check(context.inputPanel().preedit().toString() == "你", "switch-back restores composition");
  key(context, "Return");
  check(context.commits == std::vector<std::string>{"你", "你"},
        "returning after ordinary input commits only the retained composition");
  typeReading(context);
  key(context, "Escape");
  check(context.inputPanel().preedit().empty() && context.commits.size() == 2,
        "Escape explicitly cancels without inserting text");
}

static void unsupportedKeyRegression(TestContext &context) {
  for (bool completed : {false, true}) {
    context.reset();
    if (completed) typeReading(context);
    else { key(context, "e"); key(context, "l"); }
    const auto before = context.inputPanel().preedit().toString();
    const auto candidateList = context.inputPanel().candidateList();
    check(!key(context, "F11"), "unsupported F11 key down is unhandled");
    check(!key(context, "F11", true), "unsupported F11 key up is unhandled");
    check(context.inputPanel().preedit().toString() == before && context.commits.empty(),
          "unsupported F11 preserves reading or completed composition without committing");
    check(context.inputPanel().candidateList() == candidateList,
          "unsupported F11 retains candidate state");
  }
}

static void missingDictionaryRegression(Instance &instance, TestContext &context,
                                        const std::string &validPath) {
  check(context.inputPanel().auxDown().toString().find("TextMap") != std::string::npos,
        "activation error is visible and identifies TextMap; auxUp=" +
            context.inputPanel().auxUp().toString() + "; auxDown=" +
            context.inputPanel().auxDown().toString() + "; active=" + instance.inputMethod(&context));
  // reset must not create a session, even if the dictionary becomes usable.
  setenv("ZHUYINFLOW_TEXTMAP", validPath.c_str(), 1);
  context.reset();
  setenv("ZHUYINFLOW_TEXTMAP", "/definitely/missing/dictionary.txtMap", 1);
  check(!key(context, "s") && context.inputPanel().preedit().empty(),
        "reset of absent state does not construct a session");
  context.loseFocus();
  context.focusIn();
  instance.setCurrentInputMethod(&context, "keyboard-us", false);
  check(!key(context, "a"), "other input method remains usable after dictionary failure");
  setenv("ZHUYINFLOW_TEXTMAP", validPath.c_str(), 1);
  instance.setCurrentInputMethod(&context, "zhuyinflow", false);
  typeReading(context);
  key(context, "Return");
  check(context.commits == std::vector<std::string>{"你"}, "activation retries after dictionary is repaired");
}

int main(int argc, char **argv) {
  check(argc == 2, "one scenario is required");
  const std::string scenario = argv[1];
  const std::string dictionary = std::getenv("ZHUYINFLOW_TEXTMAP") ? std::getenv("ZHUYINFLOW_TEXTMAP") : "";
  check(!dictionary.empty(), "ZHUYINFLOW_TEXTMAP fixture is required");
  if (scenario == "missing-dictionary") {
    setenv("ZHUYINFLOW_TEXTMAP", "/definitely/missing/dictionary.txtMap", 1);
  } else if (scenario == "invalid-dictionary") {
    const std::string invalidPath = std::string(std::getenv("XDG_CACHE_HOME")) + "/invalid.txtMap";
    std::ofstream(invalidPath) << "This is not a factory TextMap.\n";
    setenv("ZHUYINFLOW_TEXTMAP", invalidPath.c_str(), 1);
  }
  char arg0[] = "fcitx5_regression_test";
  char arg1[] = "--disable=all";
  char arg2[] = "--enable=zhuyinflow,keyboard,testui";
  char *args[] = {arg0, arg1, arg2};
  Instance instance(3, args);
  PassThroughKeyboardFactory keyboardFactory;
  StaticAddonRegistry registry{{"keyboard", &keyboardFactory}};
  instance.addonManager().registerDefaultLoader(&registry);
  EventDispatcher dispatcher;
  dispatcher.attach(&instance.eventLoop());
  dispatcher.schedule([&]() {
    CapabilityFlags flags = CapabilityFlag::Preedit;
    if (scenario == "focus-no-preedit") flags = CapabilityFlag::NoFlag;
    if (scenario == "focus-client-commit") flags |= CapabilityFlag::ClientUnfocusCommit;
    if (scenario == "focus-formatted") flags |= CapabilityFlag::FormattedPreedit;
    if (scenario == "focus-formatted-client-commit") {
      flags |= CapabilityFlag::ClientUnfocusCommit;
      flags |= CapabilityFlag::FormattedPreedit;
    }
    TestContext context(instance.inputContextManager(), flags);
    context.focusIn();
    instance.setCurrentInputMethod(&context, "zhuyinflow", false);
    if (scenario == "candidates") candidateRegression(context);
    else if (scenario == "missing-dictionary" || scenario == "invalid-dictionary") {
      missingDictionaryRegression(instance, context, dictionary);
    }
    else if (scenario == "unsupported-key") unsupportedKeyRegression(context);
    else if (scenario == "continuous-input") continuousInputRegression(context);
    else focusRegression(instance, context);
    context.loseFocus();
    std::cout << "PASS: " << scenario << std::endl;
    dispatcher.detach();
    instance.exit();
  });
  instance.exec();
}
