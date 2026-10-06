// SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
// SPDX-License-Identifier: MulanPSL-2.0

import Foundation
import Testing

@testable import VGReplCore

@Suite
struct VGReplCoreTests {
  private func response(for json: String) throws -> VGReplResponse {
    try JSONDecoder().decode(VGReplResponse.self, from: Data(json.utf8))
  }

  @Test(
    "Dachen readings",
    arguments: [
      (["s", "u"], "ㄋㄧ"),
      (["s", "u", "3"], "ㄋㄧˇ"),
      (["c", "l", "3"], "ㄏㄠˇ"),
      (["5", "j", "/"], "ㄓㄨㄥ"),
      (["s", "u", "backspace"], "ㄋ"),
      (["s", "u", "backspace", "backspace"], ""),
      (["u", "s"], "ㄋㄧ"),
    ]
  )
  func dachenReadings(keys: [String], expected: String) throws {
    var session = try VGReplSession(layout: "dachen")
    var lastJSON = ""
    for key in keys {
      lastJSON = try session.process(key: key)
    }
    #expect(try response(for: lastJSON).composition == expected)
  }

  @Test("Shift-prefixed keys use the Dachen key")
  func shiftPrefixedKey() throws {
    var session = try VGReplSession(layout: "dachen")
    let result = try response(for: session.process(key: "shift+a"))
    #expect(result.composition == "ㄇ")
  }

  @Test("Each key emits one complete JSON line")
  func emitsCompleteJSONPerKey() throws {
    var input = ["s", "u"].makeIterator()
    var output: [String] = []
    let status = VGReplProgram.run(
      arguments: ["--layout", "dachen"],
      readLine: { input.next() },
      writeOutput: { output.append($0) },
      writeError: { _ in }
    )

    #expect(status == 0)
    #expect(output.count == 2)
    for line in output {
      let decoded = try response(for: line)
      #expect(decoded.handled)
      #expect(decoded.candidates.isEmpty)
      #expect(decoded.commit.isEmpty)
    }
  }

  @Test("Unknown key reports an error and processing continues")
  func unknownKeyDoesNotEndProgram() throws {
    var input = ["not-a-key", "s"].makeIterator()
    var output: [String] = []
    let status = VGReplProgram.run(
      arguments: ["--layout", "dachen"],
      readLine: { input.next() },
      writeOutput: { output.append($0) },
      writeError: { _ in }
    )

    #expect(status == 0)
    #expect(output.count == 2)
    let error = try #require(
      JSONSerialization.jsonObject(with: Data(output[0].utf8)) as? [String: Any]
    )
    #expect(error["error"] is String)
    #expect(try response(for: output[1]).composition == "ㄋ")
  }

  @Test("End of input exits successfully")
  func endOfInputExitsSuccessfully() {
    var output: [String] = []
    let status = VGReplProgram.run(
      arguments: ["--layout", "dachen"],
      readLine: { nil },
      writeOutput: { output.append($0) },
      writeError: { _ in }
    )

    #expect(status == 0)
    #expect(output.isEmpty)
  }

  @Test("Unknown layout exits nonzero and lists available layouts")
  func unknownLayoutListsAvailableLayouts() {
    var didReadInput = false
    var errors: [String] = []
    let status = VGReplProgram.run(
      arguments: ["--layout", "missing"],
      readLine: {
        didReadInput = true
        return nil
      },
      writeOutput: { _ in },
      writeError: { errors.append($0) }
    )

    #expect(status != 0)
    #expect(!didReadInput)
    #expect(errors.contains { $0.contains("dachen") })
  }
}
