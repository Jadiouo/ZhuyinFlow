// SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
// SPDX-License-Identifier: MulanPSL-2.0

import Foundation
import Tekkon

public struct VGReplResponse: Codable, Equatable, Sendable {
  public let handled: Bool
  public let composition: String
  public let cursor: Int
  public let candidates: [String]
  public let commit: String
}

public struct VGReplSession {
  private var keys: [String] = []

  public init(layout: String = "dachen") throws {
    guard layout == "dachen" else {
      throw VGReplError.unknownLayout(layout)
    }
  }

  public mutating func process(key: String) throws -> String {
    if key == "backspace" {
      if !keys.isEmpty { keys.removeLast() }
      return try responseJSON(handled: true)
    }

    let input: String
    if key == "space" {
      input = " "
    } else if let shiftedKey = Self.shiftedKey(from: key) {
      input = shiftedKey
    } else if key.count == 1 {
      input = key
    } else {
      return try errorJSON("Unknown key: \(key)")
    }

    let composer = Tekkon.Composer(arrange: .ofDachen)
    guard composer.inputValidityCheck(charStr: input) else {
      return try errorJSON("Unknown key: \(key)")
    }
    keys.append(input)
    return try responseJSON(handled: true)
  }

  public mutating func reset() {
    keys.removeAll()
  }

  public func currentJSON(handled: Bool = false) throws -> String {
    try responseJSON(handled: handled)
  }

  private func responseJSON(handled: Bool) throws -> String {
    var composer = Tekkon.Composer(arrange: .ofDachen)
    composer.receiveSequence(keys.joined())
    let composition = composer.getComposition()
    return try Self.encode(
      VGReplResponse(
        handled: handled,
        composition: composition,
        cursor: composition.isEmpty ? 0 : 1,
        candidates: [],
        commit: ""
      )
    )
  }

  private func errorJSON(_ message: String) throws -> String {
    try Self.encode(ErrorResponse(error: message))
  }

  private static func shiftedKey(from key: String) -> String? {
    guard key.hasPrefix("shift+"), let suffix = key.split(separator: "+", maxSplits: 1).last,
          suffix.count == 1 else { return nil }
    return suffix.lowercased()
  }

  private static func encode<Value: Encodable>(_ value: Value) throws -> String {
    let encoder = JSONEncoder()
    encoder.outputFormatting = [.sortedKeys]
    let data = try encoder.encode(value)
    return String(decoding: data, as: UTF8.self)
  }

  private struct ErrorResponse: Encodable {
    let error: String
  }
}

public enum VGReplError: Error, CustomStringConvertible {
  case unknownLayout(String)

  public var description: String {
    switch self {
    case let .unknownLayout(layout):
      "Unknown layout '\(layout)'. Available layouts: dachen."
    }
  }
}
