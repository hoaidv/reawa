---
feature: menu-bar-shell
parent_req: [REQ-01]
version: 1.0.0
lifecycle: active
---

# SRS — Menu Bar Shell (Quality)

## Prioritised quality goals

1. **Startup reliability** — No crash on non-bundled launch
2. **Responsiveness** — Menu refresh reflects status within one poll cycle

## [SRS-RW-07] Startup — non-bundled launch

| Field | Value |
|---|---|
| Source | Developer running `swift run reawa` |
| Stimulus | App launch without `.app` bundle |
| Artifact | NotificationService |
| Environment | SwiftPM debug build |
| Response | App starts; notifications suppressed; no crash |
| Response measure | 0 crashes in 100 consecutive non-bundled launches |

Constrains: [SRS-RW-04](srs-logic.md).

---

## Superseded

_None yet._
