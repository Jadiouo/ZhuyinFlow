// SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
// SPDX-License-Identifier: MulanPSL-2.0

import Foundation
import VGReplCore
import LibVanguard
import LexiconAssembly
import Shared

#if canImport(Glibc)
  import Glibc
#elseif canImport(Darwin)
  import Darwin
#endif

private struct VGBridgeResponse: Encodable {
  let handled: Bool
  let composition: String
  let cursor: Int
  let candidates: [String]
  let candidateSelectionActive: Bool
  let commit: String
}

private struct VGBridgeConfiguration: Decodable {
  let layout: String
  let lexicon: String?
  let mixedAlphanumericalEnabled: Bool?
  let furiousTypingEnabled4Zhuyin: Bool?
}

@MainActor
private final class VGBridgeSession {
  private static var activeLexiconPath: String?
  private static var activeLexiconSessionCount = 0

  private var spellingSession: VGReplSession?
  private var core: InputSession?
  private var client: VGBridgeClientProxy?
  private var controllerAddress: UInt?
  private var usesLexicon = false

  init(configuration: VGBridgeConfiguration) throws {
    guard configuration.layout == "dachen" else {
      throw VGBridgeError.invalidLayout
    }

    guard let lexicon = configuration.lexicon else {
      spellingSession = try VGReplSession(layout: configuration.layout)
      return
    }

    let lexiconPath = URL(fileURLWithPath: lexicon).standardizedFileURL.path
    if Self.activeLexiconSessionCount > 0, Self.activeLexiconPath != lexiconPath {
      throw VGBridgeError.conflictingLexiconPaths
    }
    if Self.activeLexiconPath != lexiconPath || !LXAssembly.LXFacade.isFactoryDictionaryLoaded {
      let validation = LXAssembly.LXFacade.validateFactoryTextMapFile(at: lexiconPath)
      guard validation.isValid else {
        throw VGBridgeError.invalidLexicon(validation.errorDescription ?? "Invalid TextMap.")
      }
      LXAssembly.LXFacade.asyncLoadingUserData = false
      LXAssembly.LXFacade.connectFactoryDictionary(textMapPath: lexiconPath)
      guard LXAssembly.LXFacade.isFactoryDictionaryLoaded else {
        throw VGBridgeError.failedToLoadLexicon(lexiconPath)
      }
      Self.activeLexiconPath = lexiconPath
    }

    let host = SessionHost.shared
    host.isCoreDBConnected = { true }
    host.isControllerAddressAlive = { _ in true }
    let prefs = PrefMgr.sharedSansDidSetOps
    if let mixedAlphanumericalEnabled = configuration.mixedAlphanumericalEnabled {
      prefs.mixedAlphanumericalEnabled = mixedAlphanumericalEnabled
    }
    if let furiousTypingEnabled4Zhuyin = configuration.furiousTypingEnabled4Zhuyin {
      prefs.furiousTypingEnabled4Zhuyin = furiousTypingEnabled4Zhuyin
    }
    host.prefs = { prefs }
    host.isKeyboardJIS = { false }
    host.isDynamicBasicKeyboardLayoutEnabled = { false }

    let client = VGBridgeClientProxy()
    let controllerAddress = UInt(bitPattern: Unmanaged.passUnretained(client).toOpaque())
    let core = InputSession(preallocated: (), controllerAddr: controllerAddress)
    core.constructSansAsync(clientProxy: client)
    if core.inputMode == .imeModeNULL {
      core.inputMode = .imeModeCHT
    }

    self.spellingSession = nil
    self.client = client
    self.controllerAddress = controllerAddress
    self.core = core
    Self.activeLexiconSessionCount += 1
    usesLexicon = true
  }

  func feed(_ event: KBEvent?) throws -> String {
    if var spellingSession {
      guard let event, event.type == .keyDown else {
        return try spellingSession.currentJSON()
      }
      let key: String
      if event.keyCode == KeyCode.kBackSpace.rawValue {
        key = "backspace"
      } else {
        key = event.charactersIgnoringModifiers ?? ""
      }
      let result = try spellingSession.process(key: key)
      self.spellingSession = spellingSession
      return result
    }

    guard let core, let client else {
      throw VGBridgeError.noCoreSession
    }
    InputSession.current = core
    client.clearCommit()
    // An unmapped keysym is unhandled input, not the upstream nil-event reset.
    // Preserve the snapshot and never replay a commit from the previous event.
    guard let event else {
      return try makeJSON(handled: false, commit: "")
    }
    let handled = core.handleEvent(event)
    return try makeJSON(
      handled: handled,
      commit: client.takeCommit()
    )
  }

  func selectCandidate(at index: Int32) throws -> String {
    if let spellingSession {
      return try spellingSession.currentJSON()
    }
    guard let core, let client else {
      throw VGBridgeError.noCoreSession
    }
    InputSession.current = core
    guard index >= 0, let handler = core.inputHandler, core.state.hasComposition else {
      return try makeJSON(handled: false, commit: "")
    }
    let candidateState = core.state.isCandidateContainer
      ? core.state
      : handler.generateStateOfCandidates(dodge: false)
    guard candidateState.candidates.indices.contains(Int(index)) else {
      return try makeJSON(handled: false, commit: "")
    }
    if !core.state.isCandidateContainer {
      core.switchState(candidateState)
    }
    client.clearCommit()
    core.candidatePairSelectionConfirmed(at: Int(index))
    return try makeJSON(handled: true, commit: client.takeCommit())
  }

  func reset() {
    if spellingSession != nil {
      spellingSession?.reset()
    } else {
      if let core { InputSession.current = core }
      client?.clearCommit()
      core?.switchState(.ofAbortion())
    }
  }

  func dispose() {
    if let core, let controllerAddress {
      core.switchState(.ofAbortion())
      core.inputHandler = nil
      core.recentMarkedText = (nil, nil)
      InputSession.unregisterSessionAddr(forControllerAddr: controllerAddress)
      if InputSession.current?.id == core.id {
        InputSession.current = nil
      }
    }
    if usesLexicon {
      Self.activeLexiconSessionCount -= 1
      usesLexicon = false
    }
  }

  private func makeJSON(handled: Bool, commit: String) throws -> String {
    guard let core else { throw VGBridgeError.noCoreSession }
    let candidates: [String]
    if core.state.isCandidateContainer {
      candidates = core.state.candidates.map(\.value)
    } else if core.state.hasComposition, let handler = core.inputHandler {
      candidates = handler.generateStateOfCandidates(dodge: false).candidates.map(\.value)
    } else {
      candidates = []
    }
    // Suggestions during ordinary typing are not a selection mode: Dachen
    // digit keys can start the next syllable. Only an explicit candidate state
    // lets adapters interpret those keys as labels.
    let response = VGBridgeResponse(
      handled: handled,
      composition: core.state.displayedText,
      cursor: core.state.cursor,
      candidates: candidates,
      candidateSelectionActive: !candidates.isEmpty && core.state.type == .ofCandidates,
      commit: commit
    )
    let encoder = JSONEncoder()
    encoder.outputFormatting = [.sortedKeys]
    let data = try encoder.encode(response)
    return String(decoding: data, as: UTF8.self)
  }
}

private enum VGBridgeError: Error {
  case invalidLayout
  case invalidLexicon(String)
  case failedToLoadLexicon(String)
  case conflictingLexiconPaths
  case noCoreSession
}

@MainActor
private final class VGBridgeClientProxy: NSObject, SessionClientProxy {
  private var committedText = ""

  func clearCommit() {
    committedText = ""
  }

  func takeCommit() -> String {
    defer { committedText = "" }
    return committedText
  }

  func hasClient() -> Bool { true }

  func clientTextInsertion(with text: String, replacementRange _: NSRange) {
    committedText += text
  }

  func clientMarkedTextSetup(
    with _: NSAttributedString,
    selectionRange _: NSRange,
    replacementRange _: NSRange
  ) {}

  func clientBundleIdentifier() -> String? { "org.zhuyinflow.linux.vgbridge" }

  func clientSelectMode(withModeIdentifier _: String) {}

  func clientOverrideKeyboard(withName _: String) {}

  func clientAttributesForCharacterIndex(
    atU16Pos _: UInt,
    lineHeightRectangle _: UnsafeMutablePointer<CGRect>
  ) -> [AnyHashable: Any]? { nil }

  func clientLineHeightRect(forU16CursorPos _: UInt) -> CGRect { .zero }
}

@MainActor
@_cdecl("vg_session_new")
public func vg_session_new(_ configJSON: UnsafePointer<CChar>?) -> UnsafeMutableRawPointer? {
  guard let configJSON else { return nil }
  do {
    let data = Data(String(cString: configJSON).utf8)
    let configuration = try JSONDecoder().decode(VGBridgeConfiguration.self, from: data)
    return Unmanaged.passRetained(try VGBridgeSession(configuration: configuration)).toOpaque()
  } catch {
    return nil
  }
}

@MainActor
@_cdecl("vg_session_free")
public func vg_session_free(_ session: UnsafeMutableRawPointer?) {
  guard let session else { return }
  let bridgeSession = Unmanaged<VGBridgeSession>.fromOpaque(session).takeRetainedValue()
  bridgeSession.dispose()
}

@MainActor
@_cdecl("vg_feed_key")
public func vg_feed_key(
  _ session: UnsafeMutableRawPointer?,
  _ fcitxKeyCode: UInt32,
  _ fcitxModifiers: UInt32,
  _ isKeyDown: Int32
) -> UnsafeMutablePointer<CChar>? {
  guard let bridgeSession = session.map({
    Unmanaged<VGBridgeSession>.fromOpaque($0).takeUnretainedValue()
  }) else {
    return makeCString("{\"error\":\"Invalid session\"}")
  }

  let event = KBEvent(
    fcitxKeyCode: fcitxKeyCode,
    fcitxModifierFlags: fcitxModifiers,
    isKeyDown: isKeyDown == 0 ? false : true
  )
  do {
    return makeCString(try bridgeSession.feed(event))
  } catch {
    return makeCString("{\"error\":\"Encoding failed\"}")
  }
}

@MainActor
@_cdecl("vg_select_candidate")
public func vg_select_candidate(
  _ session: UnsafeMutableRawPointer?,
  _ index: Int32
) -> UnsafeMutablePointer<CChar>? {
  guard let bridgeSession = session.map({
    Unmanaged<VGBridgeSession>.fromOpaque($0).takeUnretainedValue()
  }) else {
    return makeCString("{\"error\":\"Invalid session\"}")
  }
  do {
    return makeCString(try bridgeSession.selectCandidate(at: index))
  } catch {
    return makeCString("{\"error\":\"Encoding failed\"}")
  }
}

@MainActor
@_cdecl("vg_reset")
public func vg_reset(_ session: UnsafeMutableRawPointer?) {
  guard let bridgeSession = session.map({
    Unmanaged<VGBridgeSession>.fromOpaque($0).takeUnretainedValue()
  }) else { return }
  bridgeSession.reset()
}

@_cdecl("vg_string_free")
public func vg_string_free(_ value: UnsafeMutablePointer<CChar>?) {
  guard let value else { return }
  free(value)
}

private func makeCString(_ string: String) -> UnsafeMutablePointer<CChar>? {
  string.withCString { strdup($0) }
}
