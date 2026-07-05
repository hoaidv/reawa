---
title: PRD — Reawa
module: reawa
version: 1.0.0
lifecycle: active
parent_brd: [BRD-01, BRD-02, BRD-03]
owner: pm
---

# PRD — Reawa

Problem-first product requirements for the reMarkable → macOS pen driver. Every `[REQ-NN]` traces to a `[BRD-NN]` and has measurable acceptance criteria.

## Problem & Job-to-be-Done

Creative professionals and productivity users who own a reMarkable 2 want to use its paper-like pen surface as a Mac input device — for sketching in Figma, annotating, or navigating with hover/touch — without buying a separate drawing tablet. Today there is no native, lightweight macOS utility that streams reMarkable pen data and maps it into usable cursor or stylus input with minimal setup.

## Target Users

- **Primary:** macOS users with a reMarkable 2 who want pen-as-mouse for design/creative apps (e.g. Figma Marker tool).
- **Secondary:** Developers and power users who want diagnostics and SSH-based pen streaming transparency.
- **Future:** Digital artists who need pressure-aware stylus input in Krita, Photoshop, etc. via Native Stylus mode.

## Success Metrics

| Metric | Baseline | Target | By when | Source |
|---|---|---|---|---|
| Time to first pen-driven cursor move (Relative) | N/A | ≤ 2 min after USB plug + Accessibility grant | 2026-Q3 | Manual QA |
| Absolute mode snap-to-window success rate | N/A | 100% on primary + secondary displays | 2026-Q3 | Manual QA |
| USB auto-detect without hardcoded IP | N/A | Device found on plug when on USB subnet | 2026-Q3 | Manual QA |
| Native Stylus: Krita Tablet Tester sees pen device | Not shipped | Pen/tablet-class device visible | TBD (entitlement) | Manual QA |

## [REQ-01] Menu bar application shell {#menu-bar-shell}
- **Priority:** Must · **Traces:** [BRD-01], [BRD-02]
- The app runs as a menu bar–only utility with pen-on-tablet icon. Mode switching, connection toggling, and window re-selection are available from the menu bar without an on-screen toolbar during normal use.

**Acceptance**
- Given the app is running and no settings window is open, When the user looks at the Dock, Then no Dock icon is visible.
- Given the app is running, When the user clicks the menu bar icon, Then a menu shows Connections, Relative/Absolute mode toggles, Open, and Quit.
- Given the user selects **Open**, When the settings window appears, Then a Dock icon is shown; closing the window hides the Dock icon again.

## [REQ-02] Connection management {#connection-management}
- **Priority:** Must · **Traces:** [BRD-03]
- Users save and manage multiple reMarkable device profiles. Each profile has its own SSH key pair, optional Keychain password (first setup only), and device configuration. Only one connection is active at a time.

**Acceptance**
- Given a new device, When the user enters Name, IP, Password and clicks **Add connection**, Then a per-connection RSA key is generated and stored locally and the connection appears in the saved list.
- Given an existing connection with installed key, When the user connects, Then SSH authenticates with the key only (no password prompt).
- Given connection A is active, When the user connects to connection B, Then A is disconnected and B becomes active.
- Given a saved connection, When status is polled (~every 3 s), Then it shows one of: offline (○), online (◎), connected (●), error (✗).

## [REQ-03] Relative pen input mode {#pen-input-relative}
- **Priority:** Must · **Traces:** [BRD-01], [BRD-03]
- Pen movement is relative to the current cursor position (trackpad-like). Hover moves cursor; touch performs left-click drag. Works across multiple displays without calibration.

**Acceptance**
- Given Relative mode and an active pen stream, When the user hovers the pen, Then the cursor moves by pen deltas.
- Given Relative mode, When the user touches the surface, Then left-button down/drag/up events are posted.
- Given Relative mode, When the user lifts the pen out of proximity, Then any held button is released.
- Given another input device moves the cursor mid-gesture, When the user resumes pen movement, Then the cursor does not teleport to a stale synthetic position.

## [REQ-04] Absolute pen input with window snapping {#pen-input-absolute}
- **Priority:** Must · **Traces:** [BRD-01], [BRD-03]
- Absolute mode maps the reMarkable screen onto a region bound to a real application window, maintaining tablet aspect ratio. Entering Absolute always requires choosing a target window; cancelling reverts to Relative. Pen input is clamped to the snapped window; trackpad/mouse remain free.

**Acceptance**
- Given the user selects Absolute mode while connected, When the picker appears, Then pen input to macOS is paused and a full-desktop overlay highlights windows under the cursor.
- Given the picker is active, When the user clicks a window, Then the region snaps to that window, pen input resumes, and the menu shows **Sending to …** with the window name.
- Given the picker is active, When the user presses Esc, Then mode reverts to Relative.
- Given a snapped window, When the user moves or resizes it, Then the region overlay follows and stays aspect-locked.
- Given a snapped window, When the user minimizes it (including Stage Manager), Then the overlay hides but Absolute mode stays active.
- Given a snapped window, When the user closes it, Then the app switches to Relative mode.
- Given Absolute mode, When the user uses **Choose window** from the menu, Then the picker re-runs.

## [REQ-05] USB device discovery and auto-connect {#device-discovery}
- **Priority:** Must · **Traces:** [BRD-02], [BRD-03]
- When the reMarkable is plugged in via USB, the app scans USB Ethernet subnets and probes for SSH hosts without hardcoding IP addresses.

**Acceptance**
- Given the tablet is plugged in on USB Ethernet, When discovery runs, Then the device is found (typically `10.11.99.1`) without a preconfigured IP.
- Given a saved connection with auto-connect on and the device appears, When discovery detects it, Then the app connects automatically and shows a notification.
- Given a saved connection with auto-connect off and the device appears, Then a notification prompts manual connection.
- Given no saved connection and a device is found, Then a notification shows discovered IP address(es).
- Given an active session and the device disconnects, Then the session stops and a notification is shown.

## [REQ-06] Device configuration {#device-config}
- **Priority:** Must · **Traces:** [BRD-03]
- Per-connection pen behavior: output mode (Relative / Absolute / Native Stylus), scale, tablet orientation, border color. Settings apply immediately for existing connections.

**Acceptance**
- Given an existing connection in the settings editor, When the user changes output mode, scale, orientation, or border color, Then changes apply immediately without a Save button.
- Given tablet orientation **Gut on top** (or left/bottom/right), When pen is moved, Then motion feels natural for that physical rotation.
- Given Absolute mode with a snapped window, When the settings form is open, Then **Snapped window** shows the bound window name.

## [REQ-07] Diagnostics and logging {#diagnostics}
- **Priority:** Should · **Traces:** [BRD-03]
- Settings window provides searchable behavior log (always on) and optional pen event log for raw Linux events and gesture states.

**Acceptance**
- Given the settings window, When the user opens **App Behavior Log**, Then settings changes, mode changes, connection/SSH events, device detection, and picker lifecycle appear in a searchable log.
- Given **Pen Event Log** with **Capture pen events** off, When no toggle, Then no high-frequency pen entries are recorded.
- Given pen logging enabled, When events arrive, Then raw Linux event names, accumulated pen semantics, and gesture labels (START/MOVE/END/OUT) are shown; capability chips (e.g. `BTN_STYLUS`, `ABS_TILT_X`) are clickable to filter search.

## [REQ-08] Native Stylus pen device mode (planned) {#pen-input-native-stylus}
- **Priority:** Could · **Traces:** [BRD-02], [BRD-04]
- Planned additional output backend that publishes reMarkable pen input as a macOS generic digitizer/stylus device. Mouse emulation remains available as fallback.

**Acceptance**
- Given Native Stylus mode on a signed `.app` with approved Virtual HID entitlement, When the backend starts, Then macOS receives tablet/stylus HID reports with pressure, proximity, barrel button, and tilt when available.
- Given Native Stylus fails to start (missing entitlement, unsigned bundle, or permission denied), When the session is active, Then the app falls back to the last working mouse-emulation mode and surfaces the failure in logs/UI.
- Given Krita Tablet Tester, When Native Stylus is active, Then a stylus or tablet-class device is visible (not mouse-only).
- Given `swift run reawa`, When Native Stylus is selected, Then the feature is not activatable (documented limitation).

## [REQ-09] Permissions {#permissions}
- **Priority:** Must · **Traces:** [BRD-05]
- Accessibility is required for cursor control and window snapping. Notifications are optional.

**Acceptance**
- Given Accessibility is not granted, When the user attempts pen-driven cursor control, Then input does not reach other applications until permission is granted in System Settings → Privacy & Security → Accessibility.
- Given a bundled `.app` run, When auto-connect or device-detect events occur, Then local notifications are shown if Notifications permission is granted.

---

## Product Decisions (normative)

These choices define product behavior; technical realization is in SRS and architecture docs.

1. **Absolute mode is always window-bound.** No standalone absolute region on the desktop.
2. **Window picking is mandatory for Absolute mode.** Cancelling reverts to Relative.
3. **Pen input pauses during picking; other devices do not.**
4. **Single active connection.**
5. **Menu bar–first UI.** Settings window is for configuration only.
6. **Dock icon only while settings are open.**
7. **Per-connection SSH keys and settings.**
8. **USB discovery without hardcoded IPs.**
9. **Closing the snapped window exits Absolute mode.**
10. **Minimize hides overlay; close exits Absolute.**
11. **Native Stylus is additive.** Mouse emulation must keep working when pen-device mode is unavailable.

## Non-Goals

- Polished signed App Store / direct distribution build until Apple entitlement approval.
- macOS Markup tool for screenshots/PDFs.
- Non-macOS platforms.
- Emulating Wacom-branded device identity.

## Assumptions & Dependencies

- reMarkable 2 exposes pen events at `/dev/input/event1` over SSH as root.
- macOS presents USB Ethernet on a usable subnet (typically `10.11.99.x`; link-local `169.254.x.x` possible).
- User grants Accessibility before pen control works.
- Native Stylus depends on Apple approval of `com.apple.developer.hid.virtual.device`.

## Open Questions

- Dedicated in-app network diagnostics panel (interfaces, routes, SSH errors) — **owner:** architect — **needed by:** TBD
- DriverKit system extension fallback if CoreHID path insufficient — **owner:** architect — **needed by:** post-entitlement QA

## Linked Modules

_None — single-module product._
