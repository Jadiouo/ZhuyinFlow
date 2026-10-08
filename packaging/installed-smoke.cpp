// SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
// SPDX-License-Identifier: MulanPSL-2.0
#include <fcitx-utils/capabilityflags.h>
#include <fcitx-utils/eventdispatcher.h>
#include <fcitx-utils/macros.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>
#include <iostream>
#include <string>
#include <vector>
using namespace fcitx;
class Context final : public InputContext {
 public:
  explicit Context(InputContextManager &manager) : InputContext(manager, "package-smoke") {
    setCapabilityFlags(CapabilityFlag::Preedit);
    created();
  }
  ~Context() override { destroy(); }
  const char *frontend() const override { return "package-smoke"; }
  std::vector<std::string> commits;
 protected:
  void commitStringImpl(const std::string &text) override { commits.push_back(text); }
  void deleteSurroundingTextImpl(int, unsigned int) override {}
  void forwardKeyImpl(const ForwardKeyEvent &) override {}
  void updatePreeditImpl() override {}
};
int main(int argc, char **argv) {
  const bool expectError = argc == 2 && std::string(argv[1]) == "error";
  char arg0[] = "package-smoke", arg1[] = "--disable=all", arg2[] = "--enable=zhuyinflow";
  char *args[] = {arg0, arg1, arg2};
  Instance instance(3, args);
  instance.addonManager().registerDefaultLoader(nullptr);
  EventDispatcher dispatcher;
  dispatcher.attach(&instance.eventLoop());
  dispatcher.schedule([&]() {
    Context context(instance.inputContextManager());
    context.focusIn();
    const auto key = [&](const char *name) {
      KeyEvent event(&context, Key(name), false); context.keyEvent(event);
      KeyEvent release(&context, Key(name), true); context.keyEvent(release);
    };
    key("s"); key("u"); key("3");
    if (expectError) {
      FCITX_ASSERT(context.commits.empty());
      FCITX_ASSERT(context.inputPanel().auxDown().toString().find("TextMap") != std::string::npos);
      std::cout << "EXPECTED_DICTIONARY_ERROR\n";
    } else {
      FCITX_ASSERT(context.inputPanel().preedit().toString() == "你");
      key("Return");
      FCITX_ASSERT(context.commits == std::vector<std::string>{"你"});
      FCITX_ASSERT(context.inputPanel().preedit().empty());
      key("Return");
      FCITX_ASSERT(context.commits.size() == 1);
      std::cout << "INSTALLED_SMOKE_COMMIT_ONCE=你\n";
    }
    context.focusOut();
    dispatcher.detach();
    instance.exit();
  });
  return instance.exec();
}
