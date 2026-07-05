---
feature: diagnostics
parent_req: [REQ-07]
version: 1.0.0
lifecycle: active
---

# SRS — Diagnostics (Quality)

## Prioritised quality goals

1. **UI responsiveness under pen log flood** — Main thread not blocked

## [SRS-RW-66] Pen log main-thread isolation

| Field | Value |
|---|---|
| Source | High-frequency RM2 pen stream |
| Stimulus | Pen logging enabled during active session |
| Artifact | Settings Pen Event Log UI |
| Environment | Connected session, sustained pen movement |
| Response | UI remains interactive; log updates without stutter |
| Response measure | Main thread not blocked by per-event SwiftUI publish (debounced snapshots) |

Constrains: [SRS-RW-62](srs-logic.md).

---

## Superseded

_None yet._
