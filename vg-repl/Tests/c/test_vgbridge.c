/* SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors */
/* SPDX-License-Identifier: MulanPSL-2.0 */

#include "vgbridge.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *kConfig = "{\"layout\":\"dachen\"}";
static const char *kLexiconConfig =
    "{\"layout\":\"dachen\",\"lexicon\":\"upstream/Packages/"
    "vChewing_OSNeutral_LibVanguard/Sources/LXAssemblyMaterials4Tests/"
    "Resources/vanguardTextMap_test.txtMap\"}";
static const char *kFuriousConfig =
    "{\"layout\":\"dachen\",\"mixedAlphanumericalEnabled\":false,"
    "\"furiousTypingEnabled4Zhuyin\":true,\"lexicon\":\"upstream/Packages/"
    "vChewing_OSNeutral_LibVanguard/Sources/LXAssemblyMaterials4Tests/"
    "Resources/vanguardTextMap_test.txtMap\"}";

static char *feed(vg_session *session, uint32_t keycode) {
  return vg_feed_key(session, keycode, 0, 1);
}

static char *feed_paired(vg_session *session, uint32_t keycode) {
  char *result = feed(session, keycode);
  assert(result != NULL);
  vg_string_free(result);
  result = vg_feed_key(session, keycode, 0, 0);
  assert(result != NULL);
  assert(strstr(result, "\"handled\":true") != NULL);
  return result;
}

static char *type_tech_phrase(vg_session *session) {
  const uint32_t phraseKeys[] = {
      0x044, 0x04b, 0x020, 0x052, 0x055, 0x034,
  };
  char *result = NULL;
  for (size_t i = 0; i < sizeof(phraseKeys) / sizeof(phraseKeys[0]); ++i) {
    result = feed_paired(session, phraseKeys[i]);
    assert(result != NULL);
    if (i + 1 < sizeof(phraseKeys) / sizeof(phraseKeys[0])) {
      vg_string_free(result);
    }
  }
  return result;
}

static char *type_furious_zhuyin(vg_session *session) {
  const uint32_t keys[] = {0x045, 0x04c, 0x045, 0x04a, 0x02f};
  char *result = NULL;
  for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
    result = feed(session, keys[i]);
    assert(result != NULL);
    if (i + 1 < sizeof(keys) / sizeof(keys[0])) {
      vg_string_free(result);
    }
  }
  return result;
}

static const char *selected_phrase(const char *json) {
  if (strstr(json, "\"composition\":\"科技\"") != NULL) return "科技";
  if (strstr(json, "\"composition\":\"科際\"") != NULL) return "科際";
  assert(0 && "candidate selection did not produce a fixture phrase");
  return "";
}

static void assert_commit(const char *json, const char *value) {
  char expected[128];
  int written = snprintf(expected, sizeof(expected), "\"commit\":\"%s\"", value);
  assert(written > 0 && (size_t)written < sizeof(expected));
  assert(strstr(json, expected) != NULL);
}

int main(void) {
  assert(vg_session_new(NULL) == NULL);
  assert(vg_session_new("{") == NULL);
  assert(vg_session_new("{\"layout\":\"missing\"}") == NULL);

  vg_session *sessionA = vg_session_new(kConfig);
  vg_session *sessionB = vg_session_new(kConfig);
  assert(sessionA != NULL);
  assert(sessionB != NULL);

  char *result = feed(sessionA, 0x053);
  assert(result != NULL);
  puts(result);
  assert(strstr(result, "\"composition\":\"ㄋ\"") != NULL);
  vg_string_free(result);

  result = vg_feed_key(sessionA, 0x053, 0, 0);
  assert(result != NULL);
  assert(strstr(result, "\"composition\":\"ㄋ\"") != NULL);
  assert(strstr(result, "\"handled\":false") != NULL);
  vg_string_free(result);

  result = feed(sessionA, 0x055);
  assert(result != NULL);
  puts(result);
  assert(strstr(result, "\"composition\":\"ㄋㄧ\"") != NULL);
  vg_string_free(result);

  result = feed(sessionA, 0x033);
  assert(result != NULL);
  puts(result);
  assert(strstr(result, "\"composition\":\"ㄋㄧˇ\"") != NULL);
  vg_string_free(result);

  result = feed(sessionA, 0xff08);
  assert(result != NULL);
  assert(strstr(result, "\"composition\":\"ㄋㄧ\"") != NULL);
  vg_string_free(result);

  result = vg_feed_key(sessionB, 0x053, 0, 0);
  assert(result != NULL);
  assert(strstr(result, "\"composition\":\"\"") != NULL);
  vg_string_free(result);

  vg_reset(sessionA);
  result = vg_select_candidate(sessionA, 1);
  assert(result != NULL);
  assert(strstr(result, "\"composition\":\"\"") != NULL);
  vg_string_free(result);

  vg_session *lexiconSession = vg_session_new(kLexiconConfig);
  vg_session *secondLexiconSession = vg_session_new(kLexiconConfig);
  assert(lexiconSession != NULL);
  assert(secondLexiconSession != NULL);
  result = type_tech_phrase(lexiconSession);
  assert(strstr(result, "\"composition\":\"科技\"") != NULL);
  assert(strstr(result, "\"candidates\":[]") == NULL);
  vg_string_free(result);

  result = type_tech_phrase(secondLexiconSession);
  assert(strstr(result, "\"composition\":\"科技\"") != NULL);
  assert(strstr(result, "\"candidates\":[]") == NULL);
  vg_string_free(result);

  result = vg_select_candidate(secondLexiconSession, 0);
  assert(result != NULL);
  const char *secondSelectedPhrase = selected_phrase(result);
  vg_string_free(result);

  result = vg_select_candidate(lexiconSession, 0);
  assert(result != NULL);
  const char *firstSelectedPhrase = selected_phrase(result);
  vg_string_free(result);

  result = feed(secondLexiconSession, 0xff0d);
  assert(result != NULL);
  assert(strstr(result, "\"composition\":\"\"") != NULL);
  assert_commit(result, secondSelectedPhrase);
  vg_string_free(result);

  result = feed(lexiconSession, 0xff0d);
  assert(result != NULL);
  assert(strstr(result, "\"composition\":\"\"") != NULL);
  assert_commit(result, firstSelectedPhrase);
  vg_string_free(result);

  vg_session *furiousSession = vg_session_new(kFuriousConfig);
  assert(furiousSession != NULL);
  result = type_furious_zhuyin(furiousSession);
  assert(strstr(result, "\"composition\":\"高供\"") != NULL);
  vg_string_free(result);

  vg_session_free(furiousSession);
  vg_session_free(secondLexiconSession);
  vg_session_free(lexiconSession);
  vg_session_free(sessionA);
  vg_session_free(sessionB);

  for (int i = 0; i < 1000; ++i) {
    vg_session *session = vg_session_new(kConfig);
    assert(session != NULL);
    result = feed(session, 0x053);
    assert(result != NULL);
    vg_string_free(result);
    vg_session_free(session);
  }

  return 0;
}
