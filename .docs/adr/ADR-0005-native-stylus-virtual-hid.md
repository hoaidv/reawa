---
id: ADR-0005
title: Native Stylus virtual HID path
status: accepted
date: 2026-07-05
deciders: [architect, pm]
supersedes: null
---

# ADR-0005 — Native Stylus virtual HID path

## Context

Drawing apps need tablet/stylus-class input with pressure and tilt, not synthesized mouse events only. [REQ-08] requires a generic macOS digitizer device, SSH pen stream as source of truth, and mouse emulation fallback. Apple restricts virtual HID creation via entitlement `com.apple.developer.hid.virtual.device`.

## Decision

**Primary path:** User-space `CoreHID.HIDVirtualDevice` in `NativeStylusBackend.swift` when macOS 15+, signed `.app` bundle, approved entitlement, and post-event access granted.

**Fallback chain:**

1. Native Stylus startup failure → last working Relative or Absolute mode
2. If CoreHID path insufficient after entitlement QA → DriverKit system extension (app remains SSH/controller; extension publishes HID) — gate criteria in [native-stylus-packaging](../memory/native-stylus-packaging.md)

**Explicitly avoided:** classic kernel extensions; Wacom identity spoofing; Quartz-only path as sole drawing-app strategy.

## Consequences

- `swift run reawa` cannot test Native Stylus (unsigned, no entitlement).
- Repo includes `Config/Reawa.entitlements`, `scripts/build-macos-app.sh`, `scripts/check-native-stylus-setup.sh`.
- Validation order: Krita Tablet Tester → browser pointer-events page → Photoshop.
- Accessibility prompt is local permission only; does not replace Apple entitlement approval.

## Alternatives Considered

| Alternative | Why rejected |
|---|---|
| Quartz mouse only for drawing apps | No pressure/proximity in target apps |
| Spoof Wacom driver | Product constraint; legal/compat risk |
| DriverKit first | Heavier install/activation; app-only path tried first |
| Classic kext | Deprecated; explicit non-goal |

## Packaging follow-up (preserved)

When entitlement approved:

```bash
sh scripts/check-native-stylus-setup.sh
sh scripts/build-macos-app.sh \
  --configuration debug \
  --sign "Apple Development: YOUR NAME (TEAMID)" \
  --provisioning-profile "/path/to/Reawa.provisionprofile" \
  --show-entitlements \
  --open
```

Full checklist: [native-stylus-packaging.md](../memory/native-stylus-packaging.md).
