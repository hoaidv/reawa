# Reawa

Reawa is now a native macOS app written in Swift. It runs as a menu bar utility
that turns a reMarkable tablet into a pen input device for the Mac, with
Relative and window-bound Absolute modes, USB discovery, live SSH pen streaming,
and Accessibility-based window snapping.

The original Python app has been archived under `legacy/python/` as a reference
implementation. The active codebase is the Swift project in `Sources/ReawaApp/`.

## Repository Layout

- `Sources/ReawaApp/` — native Swift implementation
- `Tests/ReawaTests/` — parser and compatibility tests
- `Config/Info.plist` — app bundle metadata for the native app
- `Config/Reawa.entitlements` — signing/entitlement placeholder for the native bundle
- `Sources/ReawaApp/Resources/assets/` — native app assets
- `legacy/python/` — archived Python implementation and old `py2app` packaging flow
- `.docs/` — product and technical reference retained during the port

## Current Native Architecture

- `ConnectionManager.swift` handles saved connections, live session state, and
  compatibility with the previous app-support and Keychain layout.
- `SSHSession.swift` uses the system `ssh` tool to install per-device SSH keys,
  stream `/dev/input/event1`, and parse pen frames in Swift.
- `InputDrivers.swift` maps pen frames into native Quartz mouse events.
- `WindowSnap.swift` reimplements the Accessibility and `CGWindowList` logic
  needed for snapped Absolute mode, including Stage Manager-aware lifecycle checks.
- `Overlays.swift` provides the picker overlay and resizeable region overlay in AppKit.
- `SettingsUI.swift` hosts the settings and logs UI in SwiftUI.

## Compatibility

The Swift app preserves the existing user data layout where feasible:

- reads and migrates saved connections from `~/Library/Application Support/remarkable-rm2/connections.json`
- copies legacy per-connection SSH keys from `~/Library/Application Support/remarkable-rm2/keys/`
- reads passwords from both Keychain service names: `Reawa` and `remarkable-rm2`

New native data is written to:

```text
~/Library/Application Support/Reawa/
  connections.json
  keys/<connection-id>/id_rsa
  keys/<connection-id>/id_rsa.pub
```

## Run From Source

Requirements:

- macOS 13 or later
- Xcode 16 or a Swift 6 toolchain
- Accessibility permission enabled for cursor control and window snapping

Build and test:

```bash
swift test
```

Run the menu bar app from source:

```bash
swift run reawa
```

This is fine for `Relative` and `Absolute` development, but it cannot exercise
`Native Stylus`. That mode needs a signed `.app` bundle with the restricted
Virtual HID entitlement.

To package a local app bundle:

```bash
sh scripts/build-macos-app.sh --configuration debug
```

To check local signing readiness for `Native Stylus`:

```bash
sh scripts/check-native-stylus-setup.sh
```

To build and sign a bundle after Apple has approved the entitlement for your
developer team:

```bash
sh scripts/build-macos-app.sh \
  --configuration debug \
  --sign "Apple Development: YOUR NAME (TEAMID)" \
  --provisioning-profile "/path/to/Reawa.provisionprofile" \
  --show-entitlements \
  --open
```

You can also open `Package.swift` directly in Xcode. The native bundle metadata
and entitlements live in `Config/`.

## Product Behavior

The native port keeps the behavior specified in `.docs/`:

- menu bar–first workflow
- one active tablet connection at a time
- Relative mode for trackpad-like cursor movement
- Absolute mode that always snaps to a real macOS window
- pen input paused during window picking
- overlay/window lifecycle handling for minimize, restore, maximize, close, and Stage Manager

See `.docs/product.md` for the product spec and `.docs/technical.md` plus
`.docs/issues-history/macos-window-lifecycle.md` for the porting reference.

## Legacy Python App

The old Python implementation is intentionally no longer the primary workflow.
It remains in `legacy/python/` for reference while the native Swift app evolves.

## License

This project is licensed under the [MIT License](LICENSE).

See [NOTICE](NOTICE) for trademark disclaimers and attribution.
See [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md) for binary release notes.

## Trademarks

reMarkable is a registered trademark of reMarkable AS. Reawa is an independent
project and is not affiliated with, endorsed by, or sponsored by reMarkable AS.