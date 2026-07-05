---
feature: pen-input-native-stylus
parent_req: [REQ-08]
version: 1.0.0
lifecycle: active
---

# SRS — Native Stylus (Quality)

## Prioritised quality goals

1. **Graceful degradation** — Fallback when entitlement unavailable
2. **Drawing app compatibility** — Pen device visible in Krita Tablet Tester

## [SRS-RW-60] Fallback on startup failure

| Field | Value |
|---|---|
| Source | Native Stylus mode selected |
| Stimulus | Backend startup fails (no entitlement / unsigned / permission denied) |
| Artifact | DriverSession + ConnectionManager |
| Environment | `swift run` or unsigned debug bundle |
| Response | Session continues in last Relative or Absolute mode; error surfaced in log/UI |
| Response measure | 0 session crashes; pen input resumes within 1 frame after fallback |

Constrains: [SRS-RW-51](srs-logic.md).

## [SRS-RW-61] Krita Tablet Tester visibility

| Field | Value |
|---|---|
| Source | Signed app with approved Virtual HID entitlement |
| Stimulus | Native Stylus active, Krita Tablet Tester open |
| Artifact | macOS input device list |
| Environment | macOS 15+, Accessibility + post-event granted |
| Response | Tablet/stylus-class device reported |
| Response measure | Device class ≠ mouse-only (manual sign-off) |

Constrains: [SRS-RW-52](srs-logic.md).

---

## Superseded

_None yet._
