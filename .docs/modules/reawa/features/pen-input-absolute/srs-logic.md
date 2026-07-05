---
feature: pen-input-absolute
parent_req: [REQ-04]
version: 1.0.0
lifecycle: active
---

# SRS — Absolute Pen Input (Logic)

## [SRS-RW-36] AbsolutePenDriver mapping

- Maps pen `(x, y)` linearly into configured region rectangle (Quartz coords).
- Clamps synthesized cursor to region via `clamp(_:to:)`.
- Region read from live config every frame (resize/follow without reconnect).
- Hover → move; touch → left-click drag within region only.

## [SRS-RW-37] Window picker flow

Trigger: user selects Absolute (settings, menu, or auto-connect with saved Absolute config) while connection active.

1. `DriverSession.pause()` — pen frames discarded; SSH stays open ([ADR-0001](../../../../adr/ADR-0001-live-backend-swap.md)).
2. `PickerOverlayController` shows one borderless window per `NSScreen` at `NSScreenSaverWindowLevel` ([ADR-0003](../../../../adr/ADR-0003-cgwindowlist-enumeration.md)).
3. Poll `CGEventGetLocation` every 50 ms; hit-test via `windowUnderPoint(...)` (Quartz).
4. Broadcast hovered window highlight to all per-screen views (local coordinate conversion).
5. **Click** on view under cursor → `onPick(WindowInfo)` → snap → resume input → Absolute active.
6. **Esc** or cancel paths → `cancelPick()` → Relative ([SRS-RW-40](#picker-cancel-lifecycle)).

`PickerWindow.constrainFrameRect(_:to:)` returns frame unchanged (avoid AppKit shrinking away from menu bar).

## [SRS-RW-38] WindowSnapController

Uses `CGWindowListCopyWindowInfo`: layer 0, exclude own PID, min 40×40, front-to-back, Quartz bounds.

| Method | Purpose |
|---|---|
| `pick(from:)` | Resolve list entry to AX element; store title ref, window number, pid |
| `restoreWindow()` | Un-minimize/stage; raise; activate app; poll stable frame ~0.5 s |
| `snapRegionToWindow()` | Align region to window top-left; fit RM2 aspect inside bounds |
| `syncWindowToRegion()` | AX set position/size to match region (size → position → size) |
| `syncRegionToWindow()` | Align region to current window bounds (aspect-fit inside) |
| `currentWindowFrame()` | Read AX position/size for follow timer |
| `snappedLifecycleState()` | `closed` / `minimized` / `maximized` / `normal` |

AX position/size wrapped in `AXValueCreate(.cgPoint / .cgSize)` — raw structs silently fail (bug #2).

`axWindowFrame(...)` unwraps `AXValueRef` via `AXValueGetValue` (bug #4).

## [SRS-RW-39] Region overlay and resize

`RegionOverlayController`:

1. **Overlay window** — spans all displays; click-through (`ignoresMouseEvents = true`); border only (no dim).
2. **Corner handles** — 18×18 pt; aspect-locked drag → `onRegionChanged` → `syncWindowToRegion(...)`.

Overlay spans all connected displays.

## [SRS-RW-40] Picker cancel lifecycle {#picker-cancel-lifecycle}

Exiting picker via Relative toggle, **Choose window**, or failed flows must call `cancelPick()` to clear `picking` flag. Otherwise `startPick()` no-ops, input stays paused (bug #3).

## [SRS-RW-41] Window follow timer

`AppController` timer 0.4 s polls snapped AX window:

| State | Detection | Action |
|---|---|---|
| `normal` | Default on-screen | Overlay visible; region follows moves; external resize re-fits |
| `minimized` | See [SRS-RW-42](#stage-manager-detection) | Hide overlay; stay Absolute |
| `restored` | Was off-screen, back in OnScreenOnly | `syncRegionToWindow()`, show overlay |
| `maximized` | `AXFullScreen` or frame ≈ `visibleFrame` (6 pt) | Largest aspect-fit rect, `syncWindowToRegion()` |
| `closed` | AX `kAXErrorInvalidUIElement` | `revertToRelative(...)` |

## [SRS-RW-42] Stage Manager detection {#stage-manager-detection}

See [ADR-0004](../../../../adr/ADR-0004-stage-manager-lifecycle.md) and [investigation doc](../../../../memory/macos-window-lifecycle-investigation.md).

Minimize disjunction (after close check):

1. `kAXMinimizedAttribute`
2. Absent from `OnScreenOnly` (Cmd+H, off-stage)
3. `(cg_width × cg_height) / (ax_width × ax_height) < stageAreaRatio` (0.5)

Close: **AX invalidity only** — never CGWindowList disappearance.

Restore-on-pick: Stage Manager thumbnails in picker are small on-screen windows; `restoreWindow()` before snap (bug #14).

Full-screen attribute: use string `"AXFullScreen"` (bug #13).

## [SRS-RW-43] AppController orchestration

`updateOverlay()` central orchestrator: show/hide region overlay; start picker when Absolute but no `snappedConnectionID`; start follow timer when snapped.

Flags: `picking`, `snappedConnectionID`, `snappedWindowState` (`normal` | `minimized`).

---

## Bug fix history {#bug-fix-history}

Preserved from legacy technical reference. Verified working after fixes unless noted.

| # | Symptom | Root cause | Fix |
|---|---|---|---|
| 1 | Primary-display windows not pickable | Single full-desktop picker only click-interactive on display holding most area | One picker window per NSScreen |
| 2 | Snapped window never resized to region | Raw CGPoint/CGSize to AX instead of AXValueRef | AXValueCreate wrapper; size→position→size |
| 3 | Choose window / re-Absolute no-op | `picking` stayed true | `cancelPick()` in Relative/restart flows |
| 4 | Click snap stuck; no pen output | AX values not unwrapped | AXValueGetValue before read |
| 5 | Session paused after failed snap | Exception before resume | Fixed by #4; cancelPick on other exits |
| 6 | Wrong/stale Absolute region | Mode saved without successful snap | Successful snap sets region from picked bounds |
| 7 | Settings crash on reopen | Window released on close | `isReleasedWhenClosed = false` |
| 8 | Cmd+V failed in settings | No Edit menu in status bar shell | Standard Edit menu / responder chain |
| 9 | Stage Manager minimize → RELATIVE | Close via CGWindowList membership | Close via AX invalidity only |
| 10–12 | Overlay stuck / false hide (Stage Manager) | Strip-delta + frontmost heuristics | CG/AX area ratio (0.5) |
| 13 | Full-screen detection crash | Missing kAXFullScreen export | Raw `"AXFullScreen"` string |
| 14 | Stage Manager thumbnail tiny snap | Picker returns thumbnail bounds | restoreWindow() before snap |
| 15 | Relative under-scaled in Swift port | Live cursor read every frame | Gesture anchor-based deltas |
| 16 | Teleport after trackpad interference | Stale synthetic cursor | Rebase gesture to live cursor |

Full investigation narrative: [macos-window-lifecycle-investigation.md](../../../../memory/macos-window-lifecycle-investigation.md).

---

## Superseded

_None yet._
