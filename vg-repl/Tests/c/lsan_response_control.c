// SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
// SPDX-License-Identifier: MulanPSL-2.0

#include "vgbridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *config="{\"layout\":\"dachen\",\"lexicon\":\"upstream/Packages/vChewing_OSNeutral_LibVanguard/Sources/LXAssemblyMaterials4Tests/Resources/vanguardTextMap_test.txtMap\"}";
__attribute__((noinline)) static void leaked_response(vg_session *s) {
  char *json=vg_feed_key(s,0x53,0,1);
  if(!json) abort();
  fprintf(stderr,"responseLength=%zu intentionally not freed\n",strlen(json));
  __asm__ __volatile__("" : : "r"(json) : "memory");
}
int main(void) {
  vg_session *s=vg_session_new(config);
  if(!s)return 2;
  leaked_response(s);
  vg_session_free(s);
  return 0;
}
