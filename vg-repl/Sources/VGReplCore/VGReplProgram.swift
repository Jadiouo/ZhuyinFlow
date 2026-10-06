// SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
// SPDX-License-Identifier: MulanPSL-2.0

public enum VGReplProgram {
  public static func run(
    arguments: [String],
    readLine: () -> String?,
    writeOutput: (String) -> Void,
    writeError: (String) -> Void
  ) -> Int32 {
    do {
      let layout = try parseLayout(arguments)
      var session = try VGReplSession(layout: layout)
      while let key = readLine() {
        do {
          writeOutput(try session.process(key: key))
        } catch {
          writeError("Failed to encode key result: \(error)")
          return 1
        }
      }
      return 0
    } catch {
      writeError(String(describing: error))
      return 2
    }
  }

  private static func parseLayout(_ arguments: [String]) throws -> String {
    guard !arguments.isEmpty else { return "dachen" }
    guard arguments.count == 2, arguments[0] == "--layout" else {
      throw VGReplArgumentError.usage
    }
    return arguments[1]
  }
}

private enum VGReplArgumentError: Error, CustomStringConvertible {
  case usage

  var description: String {
    "Usage: vg-repl [--layout dachen]. Available layouts: dachen."
  }
}
