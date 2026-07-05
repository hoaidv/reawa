---
feature: pen-input-relative
parent_req: [REQ-03]
version: 1.0.0
lifecycle: active
---

# SRS — Relative Pen Input (Logic)

## [SRS-RW-29] PenFrame model

```swift
struct PenFrame: Equatable, Sendable {
    let tvSec: UInt32
    let tvUsec: UInt32
    let x: Int
    let y: Int
    let pressure: Int?
    let touching: Bool
    let inProximity: Bool
    let stylusButton: Bool
    let distance: Int?
    let tiltX: Int?
    let tiltY: Int?
    let rawEvents: [PenRawEvent]
}
```

Assembled from Linux `input_event` records (16 bytes LE) on `EV_SYN` + `SYN_REPORT` when X and Y known. Source: `SSHSession.swift` / `PenFrameParser`.

## [SRS-RW-30] RelativePenDriver mapping

- Compute pen deltas from successive frames.
- Apply `effectiveScale`: auto PPI / RM2 DPI (~2531) when scale nil; else configured scale.
- Apply swap/invert axis transforms from `DeviceConfig`.
- Clamp cursor to union of all display bounds (`MouseController.desktopBounds()`).
- **Hover:** `kCGEventMouseMoved`
- **Touch:** left-button down / drag / up
- **Proximity loss:** release held button

## [SRS-RW-31] Gesture lifecycle and cursor rebase

`RelativePenDriver` synthesizes gesture lifecycle from `inProximity` / `touching`:

- Hover: hover-start → hover-move → hover-end
- Touch: touch-start → touch-drag → touch-end

Each gesture captures pen anchor `(x,y)` and cursor position at gesture start. Cursor motion computed relative to anchor — **not** from live `CGEventGetLocation` every frame (avoids stale cursor when Quartz events lag).

**External input rebase:** If live cursor diverges from gesture's expected cursor (trackpad/mouse moved mid-gesture), current gesture rebases to live cursor with latest pen point as new anchor. Prevents teleport on resume (bug fixes #15, #16).

## [SRS-RW-32] DriverSession integration

See [ADR-0001](../../../../adr/ADR-0001-live-backend-swap.md). Session loop reads live config, swaps backend on mode change, honors `pause()` during Absolute picker (frames discarded, SSH open).

---

## Superseded

_None yet._
