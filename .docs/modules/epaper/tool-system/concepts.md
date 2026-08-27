---
module: epaper
lifecycle: active
owner: architect
kind: implementation-view
---

# Tool system — concepts

Vocabulary as implemented under [`epaper/drawing/tools/`](../../../epaper/drawing/tools/).
Overview: [ADR-0033](../../adr/ADR-0033-tool-abstraction.md) §1–3.

## Do not call everything a tool

The primary toolbar mixes **exclusive Modes**, **Modifiers**, and (later) settings. Forcing one
`Tool` interface recreates the mixed-logic file the refactor removed.

| Kind | Exclusive on chip? | Receives pointer lock? | Examples |
|---|---|---|---|
| **InteractionMode** | Yes (`exclusiveTool` id) | No — publishes allow-lists | `InkMode`, `SelectionMode` |
| **Operation** | No | Yes — one locked gesture | InkStroke, Lasso, Move, Resize, Navigation |
| **ToolModifier** | No | Only SecondaryDeviceModifier gates Secondary | Hand-touch armed, ink-box / connector recog |
| **ToolAction** | No | No — click → document command | Enclose, InkScale, Cut, Copy, Paste |

Chip exclusive ids (`"pen"`, `"sel_rect"`, `"sel_freeform"`) are **session strings**, not `ModeId`.
`ToolCanvasItem::syncActiveMode` maps `"pen"` → `InkMode` and `sel_*` → `SelectionMode`.

## InteractionMode

Object with `ModeId` + allow-lists. Answers: *given the active exclusive tool, which Operation
kinds may run for Primary vs Secondary?*

```text
virtual ModeId id()
virtual primaryOps() / secondaryOps()   // kinds only; hub owns the instances
virtual onSecondaryCommit(caps, info)   // default no-op; InkMode may switch exclusive tool
virtual activate / deactivate           // no Modifier injection; lists are virtual methods
```

Policy that is **Mode-specific** (example: secondary Select/Move in Ink that actually selected →
`sel_freeform`) lives on the Mode, not inside Select/Move. Those Operations are shared with
SelectionMode.

Reserved: `ModeId::Eraser` (no body yet).

## Operation

Locked gesture. Declares `OperationDescriptor` (`kind`, `matchOn` / `receive` `StrategyKind`,
`priority`, `acceptPrimary` / `acceptSecondary`). Implements one sink (`RawPointerSink`, `TapSink`,
`PinchSink`). Navigation implements RawPointer **and** Pinch (two descriptors; pinch swaps the
active one).

`match()` is side-effect free and must **not** branch on `PointerDevice`. Hit-test document
geometry, exclusive-tool id, or overlay hits.

After lock, the hub feeds the sink until up/cancel. The Op calls `HostCaps` ports (`doc`, `toolUi`,
`selection`, `ink`) and may emit a document command on commit. Commit constructs a `DocEdit`
(under [`epaper/document/operations/`](../../../epaper/document/operations/)) and sends it to
`DocContext::applyEdit` — inverse undo is `DocEdit::generateUndo`, not a string switch on
`DeviceDocument`.

## PointerDevice vs PointerRole

- **Device** — physical fact from Qt/QML: `Pen`, `Finger`, `Mouse`.
- **Role** — routing axis: `Primary` or `Secondary`, from `DeviceMap` on the hub.

Default map: Primary = Pen, Secondary = Finger. Swap later with `InputHub::setDeviceMap` (no UI
yet). Unmapped devices (e.g. Mouse today) do not match.

Operations accept **roles**. There is no `penOperations()` map and no HandTouch profile table.

## ToolModifier

Orthogonal, zero or more. Never `ChipModel.exclusive`.

- **SecondaryDeviceModifier** — `armed` + `lockedUntilLift`. When disarmed, Secondary samples do
  not match. Chip tile `tgl.hand_touch` / QML `handTouchArmed`.
- **InkBoxRecognizerModifier / ConnectorRecognizerModifier** — chip toggles + latch query. Not
  InputHub sinks; pen-up recognizer dispatch still on the Tablet ingest path.

## ToolAction

Click on selection chrome. Not a locked Operation. `visible` / `enabled` / `trigger` against
`HostCaps`. Owned by `SelectionContextBar`.

## HostCaps (capability ports)

Narrow ports, not the whole app ([ADR-0033](../../adr/ADR-0033-tool-abstraction.md) §5):

| Port | Role |
|---|---|
| `InkSink *ink` | Live stroke to Tablet (Pen waveform) |
| `DocContext *doc` | Document + gesture/command helpers |
| `ToolContext *toolUi` | Overlay chrome, panel↔world, waveform |
| `SelectionContext *selection` | Durable ids + phase |
| `setExclusiveTool` | Switch chip exclusive id |

Concrete adapters: `TabletInkSink`, `SessionDocContext`, `ToolCanvasContext`. Selection store is
`SelectionContext` itself (no extra host).

## Overlay vs document paint

| Surface | Waveform | What |
|---|---|---|
| TabletCanvas | Pen | Document raster + live ink (`StrokeCapture`) |
| ToolCanvas | Mono, or Pen while lasso-ready | Lasso/marquee, settled AABB, **live manip node** while Transforming |
| ToolLayer QML | UI | Knobs + action strip (`SelectionContextToolbar.qml`) |

Live move/resize **suppresses** the node on Tablet and paints it on ToolCanvas so the hole is
filled. Overlay is visible in Selection mode **and** whenever `SelectionPhase::Transforming`
(including a finger move that started in InkMode).

## What is not in `tools/`

- [`stroke_capture.hpp`](../../../epaper/drawing/stroke_capture.hpp) — Tablet live-ink pool; stays
  next to `TabletCanvasItem`.
- Digitizer landscape→portrait [`mapPanel`](../../../epaper/input/pen_sample.hpp) — `QtInputFilter`,
  not Tool. Finger samples are already panel; do not `mapPanel` them.
- ToolChip layout / exclusive strings — `CanvasSession` / `primary_toolbar.hpp`.
