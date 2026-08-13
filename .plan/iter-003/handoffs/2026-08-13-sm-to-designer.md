---
from: sm
to: designer
date: 2026-08-13
iter: iter-003
---

# Hand-off: SM → Designer — device selection chrome (W8)

## Context

TRACK-003 is re-sliced against ADR-0014/0015. The only `needs_design` surface in lock is
[SRS-EP-12](../../../.docs/modules/epaper/features/ink-box/srs-ui.md). Implement UI stories
(EP-018, EP-019) stay `draft` until this package is `done`.

Deprecated [ink-box-ui](../design/ink-box-ui/) solved this for a mouse and a ghost. Do not port it.
[epaper-tool-strip](../design/epaper-tool-strip/) ToolChip is composed, not redesigned; its
selection-dragging ghost scenes are withdrawn.

## Pickup

| Story | Package | Status |
|---|---|---|
| [STORY-EP-012](../stories/STORY-EP-012.md) | `.plan/iter-003/design/device-selection-chrome/` | **ready** |

## Must cover

- SelectionOverlay: bounds, resize handles (**no rotation**), `inkScaleMode` toggle, mode indicator
- States: `sel.none` · `selected` · `moving` · `resizing.with_bounds` · `resizing.fixed_ink` ·
  `deselected` · `create_refused` · `unavailable` · `reloaded`
- Live ink under the pen — **0** ghost / marquee / stand-in
- Platform: epaper-device 1872×1404, 1-bit, pen, no hover, no motion

## Spike (inside this story, not after)

| Question | Owner with you |
|---|---|
| Handle size + hit tolerance in device units | architect |
| LOD cutoff on a fixed panel (not `TILE_LOD_SCALE = 0.35`) | architect |
| Undo affordance on a three-tool chip | pm |
| Selection-create invocation | pm |
| Chrome vs dense 1-bit handwriting | designer |

Record answers in the Spec or file a `CHL-*`. Implement stories must not invent the constants.

## Parallel

`/dev` runs [STORY-EP-013](../stories/STORY-EP-013.md) (latency probe) on `epaper/` paint path —
no file conflict with the design package.

## Out of scope

- Redesigning ToolChip
- Rotation, multi-select, marquee, properties panel
- Desktop ink-box UI

## Asks

1. Execute EP-012 (`ui-spec-gate` pass; set `ui_spec` / `scenes` / `hifi`).
2. Hand back to `/sm` so EP-018/EP-019 can copy Spec paths and leave `draft`.
