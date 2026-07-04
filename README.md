# Reawa

Reawa is a native macOS menu bar app that turns a reMarkable tablet into a pen
input device for the Mac. The active implementation is the Swift app in
`Sources/ReawaApp/`; the earlier Python version remains in `legacy/python/` as
reference material.

## Purpose

Reawa exists to make a reMarkable tablet useful as a Mac input surface. The
project focuses on getting pen data from the tablet onto macOS with a workflow
that feels lightweight, direct, and practical for everyday use.

## Features

- Native macOS app written in Swift
- Menu bar-first workflow
- Move your mouse with reMarkable tablet
  - Relative mode for trackpad-like cursor movement
  - Absolute mode that targets a real window
- USB discovery and SSH-based pen streaming



## Usage

- Tested against Figma's Marker tool



## Future development

- Expose the tablet as a native pen/stylus device - In progress - Waiting for developer account with proper entitlement. This feature expects supported macOS applications can recognize the tablet as an input device.
- Add a Markup tool to macOS - So you can easily markup pictures, screenshots, PDFs on macOS.
- ... add your idea here ;)



## Testing And Ideas

Testing on different Mac and reMarkable setups would be especially helpful right now. Bug reports, UX feedback, and feature ideas are all welcome.

If you try the app, useful details to share include:

- macOS version
- reMarkable model and OS version
- which input mode you used
- what worked, what felt off, and how to reproduce problems

Ideas are welcome too, especially around pen feel, connection setup, overlays,
window behavior, and packaging.

## Run From Source

Requirements:

- macOS 13 or later
- Xcode 16 or a Swift 6 toolchain
- Accessibility permission enabled for cursor control and window snapping

Run the test suite:

```bash
swift test
```

Run the menu bar app from source:

```bash
swift run reawa
```

Notes:

- Running from source is the main supported workflow today.
- `Relative` and `Absolute` modes can be exercised from source.
- `Native Stylus` requires a signed `.app` bundle with the restricted Virtual HID entitlement.

To build a local debug app bundle:

```bash
sh scripts/build-macos-app.sh --configuration debug
```

To check local signing readiness for `Native Stylus`:

```bash
sh scripts/check-native-stylus-setup.sh
```

You can also open `Package.swift` directly in Xcode. Bundle metadata and
entitlements live in `Config/`.

## Packaged App

A polished signed `Reawa.app` is not available yet.

The project is currently waiting on Apple Developer account setup and the
signing flow needed for the restricted Virtual HID entitlement. Until that is
in place:

- source builds are the supported way to try the app
- local unsigned or debug bundles are mainly for development
- a distributable build and full `Native Stylus` support are still blocked

Once the developer account and entitlement approval are in place, the existing
packaging scripts can be used to produce a signed app bundle.

## License & Trademarks

Reawa is licensed under the [MIT License](LICENSE).

See [NOTICE](NOTICE) for trademark notices and attribution, and
[THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md) for release and distribution
notes.

reMarkable is a registered trademark of reMarkable AS. Wacom is a registered
trademark of Wacom Co., Ltd. Reawa is an independent project and is not
affiliated with, endorsed by, or sponsored by reMarkable AS, Wacom Co., Ltd.,
or Apple Inc.