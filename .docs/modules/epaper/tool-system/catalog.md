---
module: epaper
lifecycle: active
owner: architect
kind: implementation-view
---

# Tool system — catalog

What is registered **today**. Paths under [`epaper/drawing/tools/`](../../../epaper/drawing/tools/).
If you change a list in code, change this page in the same change.

## Modes

| ModeId | Class | Chip exclusive ids | primaryOps | secondaryOps | onSecondaryCommit |
|---|---|---|---|---|---|
| `Ink` | [`modes/ink_mode.hpp`](../../../epaper/drawing/tools/modes/ink_mode.hpp) | `"pen"` | InkStroke | Navigation, Select, Move | If select/move mutated a non-empty selection → `sel_freeform` |
| `Selection` | [`modes/selection_mode.hpp`](../../../epaper/drawing/tools/modes/selection_mode.hpp) | `"sel_rect"`, `"sel_freeform"` | Resize, Move, Lasso, Marquee | Navigation, Select, Move, Resize, Rotate | no-op (must not force `sel_rect` → freeform) |
| `Eraser` | [`modes/eraser_mode.hpp`](../../../epaper/drawing/tools/modes/eraser_mode.hpp) | `"erase_brush"`, `"erase_area"`, `"erase_object"` | BrushErase, AreaErase, ObjectErase | Navigation | — |

Lasso `match` requires `exclusiveTool == sel_freeform`. Marquee requires `sel_rect`. Both **accept**
Primary and Secondary, but SelectionMode does **not** list them on `secondaryOps` (commented out).
Finger therefore cannot start a new lasso/marquee; Primary (pen) can, in SelectionMode.

Rotate appears on SelectionMode `secondaryOps` only; no Operation is registered, so it never wins.

## Operations

Registered in `ToolCanvasItem::registerOperations`. One instance per kind on the hub.

| Kind | File | matchOn | receive | priority | accept P / S | Notes |
|---|---|---|---|---|---|---|
| Resize | `operations/resize_operation.hpp` | HitTarget | RawPointer | 60 | both | Knob via `overlayHitAt` / `handleFromIndex` |
| Move | `operations/move_operation.hpp` | RawPointer | RawPointer | 50 | both | `hitMoveTarget`; `TransformGesture` |
| Lasso | `operations/lasso_operation.hpp` | RawPointer | RawPointer | 40 | both | `sel_freeform` only |
| Marquee | `operations/marquee_operation.hpp` | RawPointer | RawPointer | 40 | both | `sel_rect` only |
| Navigation | `operations/navigation_operation.hpp` | RawPointer / Pinch | same | 30 | Secondary only | Empty-canvas pan after 20 mm (178 du); pinch always Finger hardware |
| Select | `operations/select_operation.hpp` | Tap | Tap | 20 | Secondary only | Pick or clear |
| InkStroke | `operations/ink_stroke_operation.hpp` | RawPointer | RawPointer | 10 | Primary only | `InkSink`; skip stylus stash if `device != Pen` |
| BrushErase | `operations/brush_erase_operation.hpp` | RawPointer | RawPointer | 20 | Primary | Ghost; `StylusHoverSink` near-circle; `erase_brush` |
| AreaErase | `operations/area_erase_operation.hpp` | RawPointer | RawPointer | 20 | Primary | Dotted freeform; polygon clip + fully-inside remove |
| ObjectErase | `operations/object_erase_operation.hpp` | RawPointer | RawPointer | 20 | Primary | Append-only dashed raster (never restroke all samples); deletion-rect outline dirty; live 80% off UI via `util/latest_job.hpp`. [ADR-0036](../../adr/ADR-0036-toolcanvas-live-overlay.md) |
| Rotate | — | — | — | — | — | enum + Mode list only |

Shared move/resize math: `operations/transform_session.hpp` (Qt-free) +
`operations/transform_gesture.hpp` (ports + 200 ms Infini preview).

Navigation empty-canvas: travel below the palm threshold is rest; a tap-scale lift on empty can
clear selection (`emptyTapClearsSelection`). Follow-blocked nav is `handtouch::onLocalNav`.

## Modifiers

| Class | File | Armed means |
|---|---|---|
| `SecondaryDeviceModifier` | `modifiers/secondary_device_modifier.hpp` | Secondary pointer ops + pinch |
| `InkBoxRecognizerModifier` | `modifiers/ink_box_recognizer_modifier.hpp` | `chip.recogInkBox` |
| `ConnectorRecognizerModifier` | `modifiers/connector_recognizer_modifier.hpp` | `chip.recogConnector` |

Umbrella: `modifiers/tool_modifier.hpp` (`armed()`).

## Actions (selection strip)

[`actions/action.hpp`](../../../epaper/drawing/tools/actions/action.hpp) — Enclose, InkScale, Cut,
Copy, Paste. Cut/Copy/Paste chrome exists; clipboard wiring may still be incomplete. Host:
`ui/selection_context_bar.*` + `ui/SelectionContextToolbar.qml`.

## Contexts

| Type | File | Role |
|---|---|---|
| `DocContext` | `contexts/doc_context.hpp` | Document + commands |
| `SessionDocContext` | `contexts/session_doc_context.hpp` | Adapter over `CanvasSession` + Tablet |
| `SelectionContext` | `contexts/selection_context.hpp` | ids, pickableId, phase |
| `ToolContext` | `contexts/tool_context.hpp` | Host ports (damage, waveform, panel↔world, `refreshChrome`) |
| `ToolContextImpl` | `contexts/tool_context_impl.*` | Adapter: forwards paint / sync / refresh to the active Mode. Owns overlay dirty-union. Zero exclusive-id compares. |
| `SelectionOverlay` | `ui/selection_overlay.*` | Host-owned selection ToolCanvasLayer (settled AABB, knobs, live fill, hits). Not a member of `ToolContextImpl`. |

Phases: Idle → Selecting → Selected → Transforming.

**Overlay policy lives on the Mode** ([principles.md](./principles.md), [ADR-0035](../../adr/ADR-0035-tool-context-is-host-ports.md)). `ToolContextImpl` does not pick an Operation to paint.

| Mode | `paintOverlay` | `syncOverlay` (attach + waveform) | `refreshChrome` |
|---|---|---|---|
| Selection | Idle: nothing. Selecting: locked Lasso/Marquee. Selected: `paintSettled`. Transforming: locked Move/Resize then `paintLiveManip`. | Overlay always on. Pen while Selecting, or Idle `sel_freeform`. Mono when Selected/Transforming. | Overlay refresh with knobs iff Selected; bar; emit; `publishOverlayHits`; sync; damage |
| Eraser | Locked op, else exclusive-armed op (`match`). Brush idle = hover circle. | Overlay on. Pen while `erase_*`. | `syncOverlay` only |
| Ink | Transforming only: locked Move then `paintLiveManip`. | Visible only while Transforming (Mono). | Overlay refresh without knobs; Transforming live dirty; sync; damage |

Operations still **tell** `setStrokeWaveform` at gesture down/up. Mode restores idle Pen after up (e.g. empty freeform). Hover is `StylusHoverSink` (enter/move/leave); hub demuxes without taking the lock.

## Selection chrome

`ui/selection_overlay.*` — selection overlay state, settled/live paint, knob hits, refuse banner.
**Who calls paint is the Mode** (`paintSettled` / `paintLiveManip`). Generic dirty-union lives on
`ToolContextImpl`. During Transforming: `handleCount = 0` (no QML knobs on the live path). Settled
knobs return on `refreshChrome` after commit.

## Tree (orientation)

```text
epaper/drawing/tools/
  strategy.hpp          PointerDevice, PointerRole, DeviceMap, PointerSample, sinks, HitRegion
  operation.hpp         OperationKind, OperationDescriptor, Operation
  mode.hpp              ModeId, InteractionMode (paintOverlay / syncOverlay / refreshChrome)
  input_hub.hpp/.cpp    Router: match/lock/feed, hover cycle, interventions, device map
  host_caps.hpp         ports: ink, doc, toolUi, selection, overlay, bar, emitChromeChanged
  interventions.hpp     PenProximity, PenDown, SecondContact
  ink_sink.hpp / tablet_ink_sink.hpp
  viewport.hpp
  modes/                ink_mode, selection_mode, eraser_mode
  operations/           ink_stroke, lasso, marquee, select, move, resize, navigation,
                        brush/area/object erase, transform_gesture, transform_session
  actions/              enclose, ink_scale, cut, copy, paste
  modifiers/            tool_modifier, secondary_device, ink_box_recognizer, connector_recognizer
  contexts/             doc, session_doc, selection, tool, tool_context_impl
  ui/                   SelectionContextToolbar.qml, selection_overlay, selection_context_bar,
                        action_list_model
```

Shared coalesce (not a Mode/Operation): [`epaper/util/latest_job.hpp`](../../../epaper/util/latest_job.hpp) — one in-flight job, at most one pending; a newer submit replaces the waiting request and cooperatively cancels in-flight.
