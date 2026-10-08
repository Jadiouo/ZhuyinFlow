// SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
// SPDX-License-Identifier: MulanPSL-2.0

#include <stdlib.h>
__attribute__((noinline)) static void leak(void) {
  void *value = malloc(41);
  __asm__ __volatile__("" : : "r"(value) : "memory");
}
int main(void) { leak(); return 0; }
