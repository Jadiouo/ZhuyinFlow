// swift-tools-version: 6.4

import PackageDescription

let package = Package(
  name: "ZhuyinFlow",
  products: [
    .executable(name: "vg-repl", targets: ["VGReplCommand"]),
    .library(name: "vgbridge", type: .dynamic, targets: ["VGBridge"]),
  ],
  dependencies: [
    .package(path: "../upstream/Packages/vChewing_OSNeutral_LibVanguard"),
  ],
  targets: [
    .target(
      name: "VGReplCore",
      dependencies: [
        .product(name: "Vanguard", package: "vChewing_OSNeutral_LibVanguard"),
      ]
    ),
    .target(
      name: "VGBridge",
      dependencies: [
        "VGReplCore",
        .product(name: "Vanguard", package: "vChewing_OSNeutral_LibVanguard"),
      ],
      publicHeadersPath: "include"
    ),
    .executableTarget(
      name: "VGReplCommand",
      dependencies: ["VGReplCore"]
    ),
    .testTarget(
      name: "VGReplCoreTests",
      dependencies: ["VGReplCore"]
    ),
  ]
)
