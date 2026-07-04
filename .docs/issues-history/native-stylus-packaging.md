# Native Stylus Packaging Notes

This document captures the packaging and entitlement follow-up for the new `Native Stylus` mode that now exists in the Swift app.

## Hard Blocker Summary

For future reference, the key constraint is:

- The repository can prepare everything locally: code, bundle layout, entitlements plist, signing scripts, and permission handling.
- But the app still **cannot create a supported macOS Virtual HID device locally by itself** until Apple approves `com.apple.developer.hid.virtual.device` for the developer team that signs the app.
- The macOS Accessibility prompt is only the local permission step after that. It does not replace Apple entitlement approval.

## Current State

- The app now contains a `CoreHID.HIDVirtualDevice`-based output backend in `Sources/ReawaApp/NativeStylusBackend.swift`.
- The backend is intentionally runtime-gated:
  - it only attempts startup on supported macOS versions
  - it checks/request post-event access through `IOHIDRequestAccess(kIOHIDRequestTypePostEvent)`
  - it falls back to the last working mouse-emulation mode if virtual stylus startup fails
- The repo is still SwiftPM-first. There is not yet a signed Xcode app target or release flow for restricted HID entitlements.

## Apple-Side Preparation

There are two separate approvals involved:

1. **Apple restricted-entitlement approval** for `com.apple.developer.hid.virtual.device`
2. **macOS Accessibility permission** that you approve locally when the signed app first tries to post virtual HID events

The second one is the normal system prompt you can approve yourself. The first one must be granted by Apple on the developer team that signs the app.

Prepare these items in the Apple Developer portal:

1. Join the Apple Developer Program if the signing team is not enrolled yet.
2. Create or reuse an App ID for `io.github.hoaidv.reawa`.
3. Request the restricted entitlement `com.apple.developer.hid.virtual.device` for that team/App ID.
4. After Apple approves it, create a macOS development provisioning profile for the same App ID.
5. Install an `Apple Development` signing certificate locally so `security find-identity -v -p codesigning` shows a usable identity.

## Shipping Requirements

The app-only CoreHID path needs all of the following before it can ship as a real feature rather than a local spike:

1. A signed `.app` target managed in Xcode.
2. Apple approval for the restricted entitlement:
   - `com.apple.developer.hid.virtual.device`
3. A release signing flow that applies the entitlement only to approved builds.
4. Manual QA on a real app bundle, not only `swift test` / `swift run`.

## DriverKit Fallback Gate

Keep the app-only CoreHID path if all of these are true:

- `Krita` Tablet Tester sees a stylus or tablet-class device rather than only mouse motion.
- Pressure and basic hover/contact semantics survive into at least one real drawing app.
- The entitlement + Accessibility flow is stable enough for direct distribution.
- Device activation and teardown are reliable across connect, disconnect, and mode-switch cycles.

Branch to a DriverKit system extension if any of these stay unresolved:

- `HIDVirtualDevice` does not surface as a usable pen device in target apps.
- Pressure, tilt, or proximity semantics are dropped in the macOS input stack.
- Apple only approves the DriverKit virtual HID route for the product.
- App-only packaging proves too fragile for install/upgrade/support.

## DriverKit Path

If the fallback gate is hit, keep the current Swift app as the controller and move only the output sink into a system extension:

- App:
  - SSH connection to the reMarkable
  - pen-frame parsing
  - UI, settings, and logging
  - runtime mode selection
- DriverKit extension:
  - virtual HID digitizer service
  - report submission into macOS
  - system-extension install / activation lifecycle

Expected entitlements for that path include:

- App target:
  - `System Extension` capability
- Driver/system-extension target:
  - `com.apple.developer.driverkit`
  - `com.apple.developer.driverkit.family.hid.virtual.device`
  - any additional HID-family entitlements required by the final implementation

## Repository Follow-Up

The repo now includes the first local-development pieces:

- `Config/Reawa.entitlements` — prepared development entitlement file
- `scripts/build-macos-app.sh` — builds a real `.app` bundle from the SwiftPM executable
- `scripts/check-native-stylus-setup.sh` — quick checklist for local signing readiness

When entitlement approval exists, the next local steps are:

1. Run `sh scripts/check-native-stylus-setup.sh`.
2. Build and sign the app bundle:

   ```bash
   sh scripts/build-macos-app.sh \
     --configuration debug \
     --sign "Apple Development: YOUR NAME (TEAMID)" \
     --provisioning-profile "/path/to/Reawa.provisionprofile" \
     --show-entitlements \
     --open
   ```

3. Confirm the executable entitlements now include `com.apple.developer.hid.virtual.device`.
4. Launch the app bundle and approve the macOS Accessibility prompt.
5. Run the manual validation checklist for:
   - first-run permission prompts
   - Native Stylus activation
   - fallback to `Relative` / `Absolute`
   - reconnect behavior after unplug/replug

## Important Local Limitation

`swift run reawa` is still useful for normal development, but it cannot test Native Stylus because the SwiftPM-launched binary is not a signed app bundle with the restricted Virtual HID entitlement.
