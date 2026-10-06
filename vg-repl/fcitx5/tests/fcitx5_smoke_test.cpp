// SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
// SPDX-License-Identifier: MulanPSL-2.0

#include <fcitx-utils/eventdispatcher.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/testing.h>
#include <fcitx/addonmanager.h>
#include <fcitx/instance.h>
#include <fcitx/inputmethodentry.h>
#include <fcitx/inputmethodmanager.h>
#include <testfrontend_public.h>

using namespace fcitx;

int main() {
  char arg0[] = "fcitx5_smoke_test";
  char arg1[] = "--disable=all";
  char arg2[] = "--enable=zhuyinflow,testfrontend,testui";
  char *argv[] = {arg0, arg1, arg2};
  Instance instance(FCITX_ARRAY_SIZE(argv), argv);
  instance.addonManager().registerDefaultLoader(nullptr);

  EventDispatcher dispatcher;
  dispatcher.attach(&instance.eventLoop());
  dispatcher.schedule([&dispatcher, &instance]() {
    const auto *entry = instance.inputMethodManager().entry("zhuyinflow");
    FCITX_ASSERT(entry);
    FCITX_ASSERT(entry->label() == "Z");
    FCITX_ASSERT(entry->icon() == "zhuyinflow");

    auto *frontend = instance.addonManager().addon("testfrontend");
    FCITX_ASSERT(frontend);
    frontend->call<ITestFrontend::pushCommitExpectation>("你");
    const auto inputContext =
        frontend->call<ITestFrontend::createInputContext>("zhuyinflow-smoke-test");

    const auto sendKey = [frontend, inputContext](const Key &key) {
      frontend->call<ITestFrontend::keyEvent>(inputContext, key, false);
      frontend->call<ITestFrontend::keyEvent>(inputContext, key, true);
    };
    sendKey(Key("s"));
    sendKey(Key("u"));
    sendKey(Key("3"));
    sendKey(Key(FcitxKey_Return));

    frontend->call<ITestFrontend::pushCommitExpectation>("film ");
    sendKey(Key("f"));
    sendKey(Key("i"));
    sendKey(Key("l"));
    sendKey(Key("m"));
    sendKey(Key(FcitxKey_space));

    dispatcher.schedule([&dispatcher, &instance, frontend, inputContext]() {
      frontend->call<ITestFrontend::destroyInputContext>(inputContext);
      dispatcher.detach();
      instance.exit();
    });
  });

  instance.exec();
  return 0;
}
