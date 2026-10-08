// SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
// SPDX-License-Identifier: MulanPSL-2.0

import Foundation
import Glibc
let mode = CommandLine.arguments[1]
let count = Int(CommandLine.arguments[2])!
weak var last: NSAttributedString?
@inline(never) func perform(_ mode: String) -> Int {
  if mode == "none" { return 0 }
  let value: NSMutableAttributedString
  if mode == "empty" { value = NSMutableAttributedString(string: "") }
  else { value = NSMutableAttributedString(string: "你") }
  last = value
  if mode == "attrs" || mode == "append" {
    value.setAttributes([NSAttributedString.Key("NSUnderline"): 1,
                         NSAttributedString.Key("NSMarkedClauseSegment"): 0],
                        range: NSRange(location: 0, length: 1))
  }
  if mode == "append" {
    let result = NSMutableAttributedString(string: "")
    result.append(value)
    return result.length
  }
  return value.length
}
var length = 0
for _ in 0..<count { length += perform(mode) }
print("completed=\(count) mode=\(mode) observedLength=\(length) objectReleased=\(last == nil)")

// LSan terminates with _exit on a leak; publish the weak-reference verdict first.
fflush(nil)
