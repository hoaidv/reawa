---
module: epaper
lifecycle: active
owner: architect
kind: implementation-view
---

# Tool system — principles

Normative for anyone editing [`epaper/drawing/tools/`](../../../epaper/drawing/tools/) or
[`toolcanvasitem.cpp`](../../../epaper/drawing/toolcanvasitem.cpp). Conceptual why:
[ADR-0033](../../adr/ADR-0033-tool-abstraction.md). Host-ports lock:
[ADR-0035](../../adr/ADR-0035-tool-context-is-host-ports.md). Live overlay paint:
[ADR-0036](../../adr/ADR-0036-toolcanvas-live-overlay.md). Inventory: [catalog.md](./catalog.md).

If this page and the code disagree, the code wins — then update this page **or** file a challenge.
Do **not** silently fork policy.

**Gate:** a change that would put Mode / chip / selection policy on `ToolContextImpl`, merge
`SelectionOverlay` into `ToolCanvasItem`, or branch `Operation::match` on `PointerDevice` **stops**.
File `.plan/iter-005/challenges/CHL-<slug>.md`. Architect amends this page / ADR-0035 / ADR-0036 only after
adopt. Same stop as SRS in [docs-first](../../../.agent/rules/docs-first.md).

## Roles

| Entity | Owns | MUST NOT |
|---|---|---|
| **InteractionMode** | Exclusive-arm policy: allow-lists, overlay attach / waveform, **when** settled vs live overlay paints, `refreshChrome` | Bind Qt; classify chips for other Modes |
| **Operation** | One locked gesture; optional unlocked `StylusHoverSink`; in-flight `paintOverlay` geometry | Chip-family catalogues; `CanvasSession` / `ToolCanvasItem` pointers |
| **InputHub** | DeviceMap, match, one lock, hover demux | Overlay compositor; exclusive-id catalogue |
| **ToolContext / ToolContextImpl** | Host ports: damage, visible, waveform, panel↔world, size / scale. Forwards paint / sync / `refreshChrome` to the active Mode | Exclusive-id string compares; knob / live-manip methods |
| **SelectionOverlay + SelectionContextBar** | Selection ToolCanvasLayer state and ToolLayer widgets | Live inside ToolContextImpl or inlined into ToolCanvasItem |
| **NodeEmphasis** | Recog blink, optional stroke-stamp, AABB highlight (ToolCanvas Mono, **`includeIds` only**). Draw-into membership does **not** stamp. | Selection knobs; document FullClear; painting overlapping neighbors; inlined into ToolCanvasItem::paint |
| **DocContext** | Document, edits, Infini preview, session exclusive id, Tablet debug | Overlay paint |
| **ToolCanvasItem** | Chip-string → `ModeId` (the **only** map); QML I/O; owns impl, overlay, bar, emphasis, modes, hub | Implement `redrawLiveManip` / `publishOverlayHits` / `showManipUnavailable` / NodeEmphasis paint |

## Relationships

- Mode → overlay **policy** (paint / sync / refresh).
- Hub → Operation **lock**.
- Operations → **HostCaps** ports only.
- Never Operation → `CanvasSession` / `ToolCanvasItem`.
- `SelectionOverlay` is **host-owned**, used by SelectionMode, InkMode (Transforming), TransformGesture, ToolActions.
- `NodeEmphasis` is **host-owned** ([CHL-0030](../../../.plan/iter-005/challenges/CHL-0030-node-emphasis.md)). Every Mode paints it (under in-flight Operation overlay). TabletCanvas does not bake blink/highlight into `m_image`. InkMode **hides** ToolCanvas while an ink stroke is active (Mono over Pen dashes live ink and `toolPaint` steals samples).

## Integration (epaper)

Cite [ADR-0019](../../adr/ADR-0019-selection-chrome-layers.md):

| Surface | Waveform | What |
|---|---|---|
| TabletCanvas | Pen | Document raster + live ink (`StrokeCapture`) |
| ToolCanvas | Mono, or Pen when Mode says so | Overlay stroke (lasso, erase ghost, settled AABB, live manip fill, NodeEmphasis blink/stamp) |
| ToolLayer QML | UI | Knobs + action strip |

`StrokeCapture` and digitizer `mapPanel` stay **outside** `tools/`.

## MUST NOT (short)

- New method on `ToolContextImpl` that names a Mode, chip id, or selection knob.
- Merge `SelectionOverlay` or `NodeEmphasis` into `toolcanvasitem.cpp`.
- `if (s.device == PointerDevice::Pen)` in `Operation::match` (except InkStroke digitizer stash).
- Chip-string → ModeId anywhere except `ToolCanvasItem::syncActiveMode`.
- Rebuild or restroke a live ToolCanvas overlay from **all** samples on pointer-move or on deletion-rect damage (append-only raster + blit; [ADR-0036](../../adr/ADR-0036-toolcanvas-live-overlay.md)).
- `drawLine` each overlay sample with a fresh `DotLine` (dash restarts; looks solid).
- Run object-erase 80% / document snapshot on the pointer callback.
- Show ToolCanvas (Mono) while an **ink** stroke is in flight.
- NodeEmphasis `DocumentRenderer` of a group AABB without `includeIds` (re-strokes neighbors).
- InPlaceDirty of a live-manip **box** without bound connector spines (origin connector remains).
