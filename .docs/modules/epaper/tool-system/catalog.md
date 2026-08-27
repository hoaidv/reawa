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
| `Eraser` | — | — | — | — | reserved |

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
| `ToolContext` | `contexts/tool_context.hpp` | Overlay ports |
| `ToolCanvasContext` | `contexts/tool_canvas_context.*` | Adapter; owns `ToolChrome` |

Phases: Idle → Selecting → Selected → Transforming.

## Selection chrome

`tool_chrome.*` — overlay state, paint, damage, hit regions. During Transforming: `handleCount = 0`
(no QML knobs on the live path). Settled knobs return on chrome refresh after commit.

## Tree (orientation)

```text
epaper/drawing/tools/
  strategy.hpp          PointerDevice, PointerRole, DeviceMap, PointerSample, sinks, HitRegion
  operation.hpp         OperationKind, OperationDescriptor, Operation
  mode.hpp              ModeId, InteractionMode, SecondaryCommitInfo
  input_hub.hpp/.cpp    Router: match/lock/feed, interventions, device map
  host_caps.hpp         InkSink, DocContext, ToolContext, SelectionContext, setExclusiveTool
  interventions.hpp     PenProximity, PenDown, SecondContact
  ink_sink.hpp / tablet_ink_sink.hpp
  viewport.hpp
  tool_chrome.hpp/.cpp
  modes/                ink_mode, selection_mode
  operations/           ink_stroke, lasso, marquee, select, move, resize, navigation,
                        transform_gesture, transform_session
  actions/              enclose, ink_scale, cut, copy, paste
  modifiers/            tool_modifier, secondary_device, ink_box_recognizer, connector_recognizer
  contexts/             doc, session_doc, selection, tool, tool_canvas
  ui/                   SelectionContextToolbar.qml, selection_context_bar, action_list_model
```
