---
title: Business Requirements Document — Reawa
version: 1.0.0
status: approved
owner: analyst
last_review: 2026-07-05
---

# BRD — Reawa

## [BRD-01] Vision

Reawa is a macOS menu bar application that turns a reMarkable 2 tablet into a pen-controlled input device for the Mac. The tablet streams pen events over USB (via SSH) and the app translates them into native mouse movement and clicks — or, in a planned future mode, into a macOS-recognized tablet/stylus device.

The product serves creative and productivity workflows where the reMarkable's paper-like surface is preferable to a trackpad or mouse: sketching in design tools, annotating documents, or navigating applications with pen hover and touch.

## [BRD-02] Strategic Goals

| Goal | Measurable Outcome | Sponsor |
|---|---|---|
| Make reMarkable useful as a Mac input surface | Users can move the cursor and click/drag from the tablet in Relative mode without calibration | Product |
| Enable precision window-targeted pen work | Users can map the tablet screen onto a real application window in Absolute mode | Product |
| Zero-friction device setup | USB-plugged tablets are discovered automatically without hardcoded IPs | Product |
| Lightweight background utility | App runs menu bar–only with no Dock icon during normal pen use | Product |
| Future: native pen recognition in drawing apps | Krita Tablet Tester sees stylus/tablet-class input (not mouse-only) when Native Stylus ships | Product |

## [BRD-03] In-Scope Domains

1. **Pen input translation** — Relative (trackpad-like) and Absolute (screen-mapped to a window) mouse emulation; planned Native Stylus backend.
2. **Device connectivity** — SSH-based pen streaming, per-connection keys, saved profiles, auto-connect.
3. **USB discovery** — Automatic detection of reMarkable on USB-tethered subnets.
4. **Window snapping** — Absolute mode bound to real application windows with lifecycle handling (move, resize, minimize, maximize, close).
5. **Configuration & diagnostics** — Settings window, behavior log, optional pen event log.

## [BRD-04] Out of Scope

- **macOS Markup tool** — Standalone markup for pictures, screenshots, PDFs (future idea from README).
- **Windows / Linux / iOS** — macOS-only product.
- **reMarkable Paper Pro / reMarkable Connect cloud** — reMarkable 2 over USB Ethernet only.
- **Kernel extensions** — Classic kext-based tablet drivers are explicitly avoided.
- **Wacom driver identity spoofing** — Generic macOS digitizer/stylus device only.
- **Polished signed distributable `.app`** — Blocked until Apple Developer account and Virtual HID entitlement approval (packaging prep exists; shipping is not in scope until approved).

## [BRD-05] Constraints

- **Platform:** macOS 13+ menu bar application; Native Stylus requires macOS 15+.
- **Hardware:** reMarkable 2 connected via USB (network over USB Ethernet; typical IP `10.11.99.1`).
- **Permissions:** Accessibility required for mouse control and window snapping; Notifications optional for auto-connect alerts.
- **Apple entitlements:** Native Stylus requires Apple-approved `com.apple.developer.hid.virtual.device`; `swift run reawa` cannot exercise this path.
- **Single active connection:** Only one device drives the cursor at a time.
- **Independent project:** Not affiliated with reMarkable AS states, Wacom, or Apple.
