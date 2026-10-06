// SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
// SPDX-License-Identifier: MulanPSL-2.0

import Foundation
import VGReplCore
import Glibc

let status = VGReplProgram.run(
  arguments: Array(CommandLine.arguments.dropFirst()),
  readLine: { Swift.readLine() },
  writeOutput: { print($0) },
  writeError: { FileHandle.standardError.write(Data(($0 + "\n").utf8)) }
)
exit(status)
