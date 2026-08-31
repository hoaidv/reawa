---
module: epaper
lifecycle: active
owner: architect
kind: implementation-view
---

# Tool system — architect reference

How exclusive tools, pointer gestures, and chrome compose on Epaper. **A view, not a spec:** it
defines no requirement ids. Product behaviour lives in [SRS-EP-04](../features/tool-modes/srs-logic.md)
(tool state / routing), [SRS-EP-11](../features/ink-box/srs-logic.md) (hit-test + transforms),
[SRS-EP-12](../features/ink-box/srs-ui.md) (selection chrome), [SRS-EP-21](../features/ink-box/srs-logic.md)
(one-finger), [SRS-EP-24](../features/region-sync/srs-logic.md) (two-finger).

**Source of truth for types and lists:** [`epaper/drawing/tools/`](../../../epaper/drawing/tools/).
If this folder and the code disagree, the code wins — then update these pages.

**Read first:** [principles.md](./principles.md) — MUST / MUST NOT for `tools/` and
`toolcanvasitem.*`. Conceptual why: [ADR-0033](../../adr/ADR-0033-tool-abstraction.md). Host-ports
lock: [ADR-0035](../../adr/ADR-0035-tool-context-is-host-ports.md). Live overlay:
[ADR-0036](../../adr/ADR-0036-toolcanvas-live-overlay.md). Names in ADR-0033 are slightly
older; this folder uses **current** code names.

## Read this folder

| Page | Use when |
|---|---|
| [principles.md](./principles.md) | Working contract — Mode policy, host ports, SelectionOverlay |
| [concepts.md](./concepts.md) | What is a Mode vs Operation vs Modifier vs Action vs Role |
| [routing.md](./routing.md) | How a pointer sample becomes a locked Operation; DeviceMap; interventions; overlay |
| [catalog.md](./catalog.md) | What is registered today (files, allow-lists, priorities) |
| [extending.md](./extending.md) | Adding a Mode, Operation, Action, or Modifier without mixing logic |

Related views:

- [event-flow.md](../features/event-system/event-flow.md) — Qt filter + QML handlers (before the hub)
- [ADR-0019](../../adr/ADR-0019-selection-chrome-layers.md) — TabletCanvas / ToolCanvas / ToolLayer
- [architecture.md](../architecture.md) — module quality goals (ink latency outranks everything)

Engineering narratives in [`.docs/memory/`](../../memory/README.md) (`plan_epaper-tool-system-refactor`,
dissolve-host-bags, ToolAction) are history. They are not the catalog.

## ADR-0033 → code names

| ADR-0033 (overview) | Code today |
|---|---|
| Pen Mode | `InkMode` (`ModeId::Ink`); chip exclusive id still `"pen"` |
| HandTouch as a Mode-profile map + `postHandling` | `SecondaryDeviceModifier` + Mode `primaryOps`/`secondaryOps` + `onSecondaryCommit` |
| Finger vs Pen as the routing axis | `PointerRole` (`Primary`/`Secondary`) via `DeviceMap`; QML still stamps `PointerDevice` |
| Transform as exclusive Mode | Move / Resize **Operations** on Ink and Selection Modes |
| Interaction Router | `InputHub` hosted by `ToolCanvasItem` |
| ToolCanvasContext | `ToolContextImpl` (host ports only) |
| ToolChrome | `SelectionOverlay` (host-owned; not inlined into the item) |

## Standing rules

1. **Not everything on the ToolChip is a Mode.** Recognizers and the hand-touch toggle are Modifiers.
2. **Move / Resize / Lasso are Operations, not exclusive tools.** The chip never grows a “Move” tile.
3. **Operations do not bind Qt.** QML stamps `PointerDevice`; the hub stamps `PointerRole`; the Op
   matches geometry / exclusive-tool id, not Pen vs Finger.
4. **One lock.** The hub matches, locks one Operation, feeds it until up/cancel.
5. **Live ink paints TabletCanvas** (`InkSink`). Overlay stroke paints ToolCanvas (`ToolContext`
   host ports). Selection knobs / live-manip live on `SelectionOverlay`, not on `ToolContextImpl`.
6. **Chip-string → `ModeId` only in `ToolCanvasItem::syncActiveMode`.** Violate [principles.md](./principles.md)
   → file a challenge; do not silently fork.
