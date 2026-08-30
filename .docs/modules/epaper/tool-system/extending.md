---
module: epaper
lifecycle: active
owner: architect
kind: implementation-view
---

# Tool system — extending

Cookbook. Code lives in [`epaper/drawing/tools/`](../../../epaper/drawing/tools/). Concepts:
[concepts.md](./concepts.md). Routing: [routing.md](./routing.md). Inventory: [catalog.md](./catalog.md).

If the change would violate an SRS, stop and file a challenge — do not silently fork policy in an
Operation `match()` on `PointerDevice`.

The architectural test from [ADR-0033](../../adr/ADR-0033-tool-abstraction.md): **adding a tool
should primarily add code**; it should not require a new branch in the router, `ToolCanvasContext`,
or a sibling Operation. Overlay **policy** (when to paint, Pen vs Mono) belongs on the active Mode.

## Add an Operation (most common)

Example: Eraser stroke, Rotate, connector-endpoint drag.

1. Add `OperationKind` in [`operation.hpp`](../../../epaper/drawing/tools/operation.hpp).
2. New header under `operations/` (`@implements` the relevant SRS). Implement one sink. Set
   `acceptPrimary` / `acceptSecondary`. `match()` uses geometry / exclusive id / overlay hits —
   **not** Pen vs Finger.
3. Pick `priority` against the catalog (Resize 60 … InkStroke 10). Canvas-wide ops that must lose
   to Move on a node stay **below 50**.
4. Append the kind to `kMatchOrder` in `input_hub.cpp` if it should participate in pointer match
   (pinch Navigation is dispatched specially and does not need this for pinch).
5. `ToolCanvasItem::registerOperations` — `setOperation(kind, make_unique<…>(&caps))`.
6. Add the kind to the right Mode lists (`primaryOps` and/or `secondaryOps`). **One instance** on
   the hub; Modes only name kinds.
7. If it paints live chrome, `paintOverlay` and/or `ToolContext` damage. Mode decides when to
   call that paint. Pen-near (unlocked) uses `StylusHoverSink`, not fields on `Operation`. Live
   node on Tablet must use suppress + ToolCanvas (see TransformGesture).
8. Update [catalog.md](./catalog.md). ARM `build-warn`; host tests if Qt-free math.

**Dual-assign (e.g. EraserMode + finger erase in InkMode):** register once; put `Eraser` on
`EraserMode.primaryOps` and on `InkMode.secondaryOps`. Navigation is priority 30 and matches all
RawPointer — a full-canvas eraser at priority 10 **never wins** on Secondary unless you drop
Navigation from that list or raise eraser above 30 (and accept losing pan). Tap-Select can coexist
(different strategy).

## Add an exclusive Mode

1. `ModeId` in [`mode.hpp`](../../../epaper/drawing/tools/mode.hpp).
2. `modes/<name>_mode.hpp` with `primaryOps` / `secondaryOps` plus `paintOverlay` / `syncOverlay`.
   Put Mode-only policy there (phase, exclusive chip, waveform), not inside `ToolCanvasContext`.
3. Chip exclusive string in `CanvasSession` / `primary_toolbar.hpp` / Main.qml tile.
4. `ToolCanvasItem::syncActiveMode` — map that string to the Mode object.
5. Do **not** make recognizers or hand-touch exclusive Modes ([ADR-0033](../../adr/ADR-0033-tool-abstraction.md)).

`activate(HostCaps &, InputHub &)` has no Modifier parameter. Lists are methods; the hub reads them
from `activeMode()`.

## Add a ToolAction

1. `actions/<name>_action.hpp` extending `ToolAction`.
2. Register on `SelectionContextBar` (or a future bar). Visibility/enabled from `HostCaps`
   (selection ids, node capabilities).
3. `trigger` calls `DocContext` / session — no pointer lock.

## Add a Modifier

1. Extend `ToolModifier` (`armed()`). Put it in `modifiers/`.
2. If it changes **who gets Secondary events**: that is `SecondaryDeviceModifier` (already the
   chip toggle). Do not add a second armed gate.
3. If it changes **pen-up document pipeline**: recognizer-style; query on commit; not an InputHub
   sink.
4. Keep chip state on `CanvasSession` / `ChipModel` unless you have a reason to move it.

## Adjust routing without new types

| Want | Where |
|---|---|
| Finger inks in InkMode | `InkMode.secondaryOps` includes InkStroke; `InkStroke` `acceptSecondary`; `match` stays device-free; keep Secondary **armed**; do not clear the modifier |
| Invert Pen/Finger | `hub.setDeviceMap({ Finger, Pen })` — no Op edits. Pinch stays Finger hardware. `PenProximity` still stylus |
| Allow finger lasso in Selection | Uncomment Lasso/Marquee on `SelectionMode::secondaryOps`; `match` already accepts Secondary |
| Smoother live manip | Overlay redraw is per-sample; Infini preview is 200 ms. Do not drive QML knobs during Transforming |
| Switch Mode after Secondary pick | `InkMode::onSecondaryCommit`, not Move/Select |

## Anti-patterns

- `if (s.device == PointerDevice::Pen)` inside `Operation::match` or gesture bodies (except
  InkStroke **stash**, which is digitizer channels, not routing).
- A second `InputHub` or per-knob QML `DragHandler` on selection chrome ([ADR-0019](../../adr/ADR-0019-selection-chrome-layers.md)).
- Giving an Op `CanvasSession *` / `TabletCanvasItem *` — use `HostCaps`.
- Merging `StrokeCapture` into `InkSink` or `TransformSession` into `TransformGesture` without a
  new ADR: one is Qt-free math/pool, the other is host ports + e-ink timing.
- Showing live manip only when `isSelectionTool`: Transforming in InkMode must still show
  ToolCanvas or the suppressed node disappears.
- Treating `.docs/memory/plan_epaper-tool-system-refactor.md` as the catalog (HandTouch profiles,
  TransformMode, PenMode).
