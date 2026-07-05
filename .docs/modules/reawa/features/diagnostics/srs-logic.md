---
feature: diagnostics
parent_req: [REQ-07]
version: 1.0.0
lifecycle: active
---

# SRS — Diagnostics (Logic)

## [SRS-RW-62] AppLogger dual channels

`AppLogger` (`Logging.swift`):

| Channel | Default | Content |
|---|---|---|
| `behaviorEntries` | Always on | Settings/mode changes, connection/session/SSH, notifications, device detection, Absolute picker/lifecycle |
| `penEntries` | Off | Raw Linux events, accumulated pen semantics, gesture labels |
| `penLoggingEnabled` | false | Runtime toggle |
| `penCapabilityLabels` | derived | Observed families: `BTN_STYLUS`, `ABS_TILT_X`, `ABS_DISTANCE`, etc. |

Pen logs appended via locked background-safe store; debounced publish to main-actor SwiftUI snapshots (avoid flooding main thread).

## [SRS-RW-63] Pen event presentation

When pen logging enabled, entries include:

- Raw Linux names: `EV_KEY BTN_STYLUS 1`
- Semantic state: `PEN TOUCH (x, y) = (...)`
- Gesture labels: `START`, `MOVE`, `END`, `OUT`
- Capability chips clickable in UI to prefill search filter

## [SRS-RW-64] Future diagnostics gap (documented)

Not yet implemented — tracked in PRD open questions:

- Active interfaces list
- USB interface selection
- Generated candidate IPs
- Route/reachability for saved device IP
- Latest SSH connection error panel

Porting notes: [swift-porting.md](../../../../memory/swift-porting.md#known-gaps).

---

## Superseded

_None yet._
