// swift-tools-version: 6.0
import PackageDescription

let package = Package(
    name: "Reawa",
    defaultLocalization: "en",
    platforms: [
        .macOS(.v13),
    ],
    products: [
        .executable(
            name: "reawa",
            targets: ["ReawaApp"]
        ),
    ],
    targets: [
        .executableTarget(
            name: "ReawaApp",
            path: "Sources/ReawaApp",
            resources: [
                .copy("Resources"),
            ]
        ),
        .testTarget(
            name: "ReawaTests",
            dependencies: ["ReawaApp"],
            path: "Tests/ReawaTests"
        ),
    ]
)
