---
feature: pen-input-native-stylus
parent_req: [REQ-08]
version: 1.0.0
lifecycle: active
---

# SRS — Native Stylus (UI)

## [SRS-RW-58] Output mode control

Settings segmented control includes **Native Stylus** alongside Relative and Absolute.

When selected, `ConnectionManager` reports native stylus status:

- Show capability / availability state in settings (startup errors, entitlement missing).
- On failure, user sees fallback to last mouse mode; behavior logged.

## [SRS-RW-59] Validation targets (QA checklist)

Recommended manual validation order:

1. **Krita** — Tablet Tester: pen/tablet device visible (not mouse-only)
2. Browser pointer-events test page — pressure/tilt sanity where supported
3. **Photoshop** or production drawing app — end-to-end after Krita passes

---

## Superseded

_None yet._
