---
feature: pen-input-absolute
parent_req: [REQ-04]
version: 1.0.0
lifecycle: active
---

# SRS — Absolute Pen Input (Quality)

## Prioritised quality goals

1. **Multi-display picker correctness** — All displays pickable
2. **Lifecycle correctness** — Stage Manager minimize/restore/close scenarios

## [SRS-RW-48] Multi-display window pick

| Field | Value |
|---|---|
| Source | User entering Absolute mode |
| Stimulus | Click window on primary display (with secondary connected) |
| Artifact | PickerOverlayController |
| Environment | "Displays have separate Spaces" enabled |
| Response | Click registers; window snaps; pen maps to region |
| Response measure | 100% success on primary + secondary in manual matrix |

Constrains: [SRS-RW-37](srs-logic.md).

## [SRS-RW-49] Stage Manager R1/R2 scenarios

| Field | Value |
|---|---|
| Source | Snapped window in Stage Manager group |
| Stimulus | R1: minimize other window then target; R2: focus sibling while target visible |
| Artifact | Region overlay visibility |
| Environment | Stage Manager enabled, 2+ windows same app/group |
| Response | Overlay hides only when **target** staged; not on unrelated focus change |
| Response measure | Pass both R1 and R2 manual scripts |

Constrains: [SRS-RW-42](srs-logic.md#stage-manager-detection).

## [SRS-RW-50] Close reverts to Relative

| Field | Value |
|---|---|
| Source | User closes snapped application window |
| Stimulus | Window destroyed |
| Artifact | AppController mode state |
| Environment | Absolute mode, normal window |
| Response | Mode → Relative; overlay removed; menu updated |
| Response measure | ≤ 1 follow timer tick (0.4 s) + UI refresh |

Constrains: [SRS-RW-41](srs-logic.md).

---

## Superseded

_None yet._
