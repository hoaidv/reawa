# macOS Window Lifecycle Detection — Investigation & Design

This document records how the `reawa` app detects **minimize**, **maximize**, **close**, and **restore** for a snapped target window under normal macOS window management and **Stage Manager**. It is written as a porting and debugging reference: what signals exist, which ones lie, and how we arrived at the current implementation.

For the concise API summary, see [pen-input-absolute SRS — Window follow](../modules/reawa/features/pen-input-absolute/srs-logic.md#window-follow-timer). For the bug-fix table, see [Bug fix history](../modules/reawa/features/pen-input-absolute/srs-logic.md#bug-fix-history) entries #9–#14.

---

## Product requirements

When a window is snapped in **ABSOLUTE** mode:

| User action | Expected app behavior |
|-------------|----------------------|
| **Minimize** (classic, Cmd+H, or Stage Manager strip) | Hide region overlay; stay in ABSOLUTE; pen mapping unchanged |
| **Restore** from minimize | Re-sync region from window frame; show overlay |
| **Maximize** (green zoom or fullscreen) | Fit largest RM2-aspect region inside screen/window; resize window to match |
| **Close** window | Revert to RELATIVE mode |
| **Move / resize** window (normal use) | Keep overlay and window in sync |

Stage Manager is the hard case: the OS does not expose a single “staged” flag on the target window, and many standard signals stay unchanged while the window is visually in the strip.

---

## Architecture overview

```
rumps.Timer (0.4 s)  →  app._follow_tick()
                            │
                            ▼
              WindowSnapController.snapped_lifecycle_state()
                            │
         ┌──────────────────┼──────────────────┐
         ▼                  ▼                  ▼
     closed            minimized          maximized / normal
  (_element_alive)   (see below)      (AX fullscreen / visibleFrame)
         │                  │
         ▼                  ▼
  _revert_to_relative   region_overlay.hide()
```

**Minimize** is a disjunction of three independent checks (first match wins after close):

1. `kAXMinimizedAttribute` — classic minimize / Cmd+M
2. `window_is_onscreen(number)` — absent from `kCGWindowListOptionOnScreenOnly` (Cmd+H, off-stage Space)
3. `_is_stage_manager_minimized()` — CG on-screen area ≪ AX frame area (Stage Manager strip)

**Close** uses AX only: `_element_alive()` returns false when `kAXRoleAttribute` yields `kAXErrorInvalidUIElement`.

---

## Coordinate spaces and APIs used

| API | What it reports for a staged window |
|-----|-------------------------------------|
| **AX** `kAXPositionAttribute` / `kAXSizeAttribute` | Often **unchanged** full-size frame |
| **AX** `kAXMinimizedAttribute` | Usually **false** under Stage Manager |
| **CGWindowList** `OnScreenOnly` | Window **still listed**; membership does not drop |
| **CGWindowList** per-window bounds | **Shrinks** to strip thumbnail (reliable) |
| **CGWindowList** `All` | Window may disappear from list (unreliable for close) |
| **NSWorkspace** frontmost app | Changes on focus; **not** staging-specific |

**Design rule:** store and compare geometry in **Quartz** global coordinates (top-left origin). Convert to Cocoa only for AppKit drawing.

---

## Investigation timeline (what we tried)

### Phase 1 — Obvious signals (rejected)

| Signal | Hypothesis | Result |
|--------|------------|--------|
| `kAXMinimizedAttribute` | Stage Manager sets minimized | **Rejected** — stays `false` when staged |
| Absent from `OnScreenOnly` list | Staged = off-screen | **Rejected** — target stays in list |
| Absent from `All` list | Staged = gone | **Rejected for close** — also absent when staged; caused false **RELATIVE** revert |
| `kAXErrorInvalidUIElement` | Close detection | **Confirmed for close** — only reliable close signal |
| `kCGWindowIsOnscreen` flag | Per-window on-screen | **Rejected** — frequently `None` under Stage Manager |
| Full AX attribute dump | Hidden attribute flips | **Rejected** — only pointer addresses change; values identical |

**Lesson:** Stage Manager keeps the target window “logically on screen” in CGWindowList while visually in the strip. AX frame stays full-size.

### Phase 2 — Window-list membership for minimize

Using `OnScreenOnly` absence worked for **Cmd+H** and classic off-screen cases but **not** for Stage Manager minimize (window remains listed).

### Phase 3 — Strip-thumbnail delta + frontmost (rejected)

**Idea:** At snap time, record `WindowManager`-owned strip window numbers on the target display. On each tick, if **new** strip thumbnails appear **and** the target app is not frontmost **and** `AXMain` → staged.

**Runtime evidence (compressed timeline from debug logs):**

- When target staged: new strip nums appear (e.g. `813`, `10068`), but sometimes `target_is_frontmost` stays **true** (R1: other window minimized first → nothing else took focus) → **overlay stuck**.
- When target still visible: `new_strip` non-empty from **unrelated** windows + `target_is_frontmost=false` after focusing another window in the same group (R2) → **false hide**.

**Lesson:** Strip deltas are **global to the display**, not **target-specific**. Frontmost is **focus**, not **staging**.

### Phase 4 — CG vs AX area ratio (current fix)

**Idea:** Compare the target’s live **CGWindow** on-screen bounds to its **AX** frame area.

| State | CG bounds (example) | AX frame (example) | Area ratio |
|-------|---------------------|--------------------|------------|
| Visible | 1428×1071 | 1428×1071 | ≈ 1.0 |
| Staged to strip | 163×164 or 98×105 | 1428×1071 | ≈ 0.017 |
| Mid-restore animation | 1172×897 | 1428×1071 | ≈ 0.69 |

**Threshold:** `_STAGE_AREA_RATIO = 0.5` — staged when `(cg_width × cg_height) / (ax_width × ax_height) < 0.5`.

**Implementation:**

```python
def window_cg_onscreen_bounds(number) -> (x, y, w, h) | None:
    # Scan kCGWindowListOptionOnScreenOnly for this kCGWindowNumber

def _is_stage_manager_minimized(self) -> bool:
    ax_frame = window_frame(self._snapped_window)      # AX
    cg_bounds = window_cg_onscreen_bounds(self._snapped_number)  # CG
    return (cg_area / ax_area) < _STAGE_AREA_RATIO
```

**Verified:** R1 (minimize other, then target) and R2 (three windows in group; focus sibling) both pass — overlay hides only when the **target** thumbnail shrinks, not when siblings move or focus changes.

---

## Other lifecycle behaviors (brief)

### Maximize

- `AXFullScreen` attribute (string `"AXFullScreen"` — not all PyObjC builds export `kAXFullScreenAttribute`)
- **Or** window frame ≈ `NSScreen.visibleFrame` on the display under window center (6 pt tolerance)
- Action: `snap_region_to_window()` then `sync_window_to_region()`

### Restore from minimize

- App-level `_snapped_window_state` tracks `"normal"` | `"minimized"` so overlay toggles only on transitions
- On restore: `sync_region_to_window()`, `region_overlay.show()`
- `_update_overlay()` skips `show()` while lifecycle reports minimized

### Restore-on-pick (Stage Manager thumbnail in picker)

Strip thumbnails are real on-screen windows in the picker. On pick:

1. `pick_from_info()` — bind AX element + window number
2. `restore_window()` — `AXMinimized=false`, `kAXRaiseAction`, `NSRunningApplication.activateWithOptions_(NSApplicationActivateAllWindows)`
3. `_await_stable_frame()` — poll until size stabilizes (~0.5 s)
4. Snap region to settled full-size frame

### Close

Never use CGWindowList disappearance for close under Stage Manager. Use:

```python
err, _ = AXUIElementCopyAttributeValue(window, kAXRoleAttribute, None)
closed = (err == kAXErrorInvalidUIElement)
```

Treat `kAXErrorCannotComplete` as still alive (transient busy app).

---

## Debugging methodology (for future macOS changes)

When lifecycle detection breaks after an OS update:

1. **Reproduce with one app instance** — duplicate `python -m reawa` processes double the follow timer and confuse logs.
2. **Log per tick for the target only:** `stored_number`, AX frame, CG on-screen bounds, `ax_minimized`, `onscreen_list` membership, `ax_main`, `target_is_frontmost`, lifecycle result.
3. **Build a compressed timeline** — print rows only when any key tuple changes; note timestamps for user actions (minimize, focus switch, restore).
4. **Separate hypotheses:**
   - Target-specific vs global (strip count, frontmost)
   - AX vs CG vs window-list
   - Close vs minimize vs focus
5. **Do not fix without a discriminating signal** — e.g. frontmost alone cannot distinguish staging from click-away; strip delta alone cannot distinguish target vs sibling staging.

Suggested log fields (NDJSON) for one investigation session:

```json
{
  "number": 9849,
  "ax_frame": [-307, -1357, 1428, 1071],
  "cg_bounds": [-582, -818, 98, 105],
  "area_ratio": 0.017,
  "ax_minimized": false,
  "onscreen_list": true,
  "target_is_frontmost": true,
  "lifecycle": "minimized"
}
```

---

## Known edge cases and limitations

- **Mid-restore animation:** Area ratio may sit between 0.5 and 1.0 briefly; overlay may flicker if threshold is too high. Tune `_STAGE_AREA_RATIO` if needed.
- **Same-app multiple windows:** CG/AX ratio is per **snapped window number**; siblings do not affect detection.
- **Apps with non-standard AX frames:** If AX reports zero size, ratio check is skipped (not staged).
- **Fullscreen spaces / Stage Manager policy changes:** Apple may change WindowManager layering; re-run the timeline procedure above.
- **Window close while staged:** AX invalidity still fires correctly once the element is destroyed.

---

## File reference

| File | Role |
|------|------|
| `Sources/ReawaApp/WindowSnap.swift` | `snappedLifecycleState()`, Stage Manager ratio, `windowCGOnscreenBounds()`, `restoreWindow()`, `stageAreaRatio` |
| `Sources/ReawaApp/AppController.swift` | Follow tick, `_snappedWindowState`, overlay show/hide on lifecycle transitions |
| `Sources/ReawaApp/Overlays.swift` | Overlay visibility only (no lifecycle logic) |

---

## Related documents

- [Module architecture](../modules/reawa/architecture.md) — full system design
- [PRD — Absolute mode](../modules/reawa/prd.md#pen-input-absolute) — user-facing behavior for minimize/maximize/close/restore
- [ADR-0004 — Stage Manager lifecycle](../adr/ADR-0004-stage-manager-lifecycle.md)
