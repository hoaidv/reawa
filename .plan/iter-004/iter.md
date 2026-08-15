---
iter: iter-004
goal: "On-device connectors (REQ-09) + ToolChip 3 tools / 2 recognizer toggles / Undo+Redo (REQ-03)"
start: 2026-08-14
end: ""
capacity: 32
committed_points: 34
status: active
---

# Iter 004 — On-device connectors

Recognize hand-drawn connectors between SmartGroups and keep them attached when a node moves.
ToolChip becomes **Rect | Freeform | Pen ⟨space⟩ Ink-box recognizer | Connector recognizer ⟨space⟩ Undo | Redo**.
Default routing is computed from the rest spine → **Ink** (`morph`) or **Curve** (`cubic`).

Lock: **vertical · verified · wip 2**. Design first in W1; `/qa` BDD in parallel; implement follows `depends_on` + BDD.

Track: [TRACK-004](../tracks/TRACK-004-on-device-connectors.md) ·
Board: [execution-board](./execution-board.md)

Prior campaign (Epaper owns the document, REQ-04…07) **closed** in
[iter-003](../iter-003/iter.md) · [retro](../iter-003/retro.md).

## Committed

- [STORY-EP-026](./stories/STORY-EP-026.md) — design — designer — 3 pts — ToolChip 3+2+Undo/Redo
- [STORY-EP-027](./stories/STORY-EP-027.md) — design — designer — 3 pts — connector blink + Ink/Curve chrome
- [STORY-EP-028](./stories/STORY-EP-028.md) — implement — dev — 3 pts — depends_on EP-026
- [STORY-EP-029](./stories/STORY-EP-029.md) — implement — dev — 5 pts — depends_on EP-028
- [STORY-EP-030](./stories/STORY-EP-030.md) — implement — dev — 5 pts — depends_on EP-027, EP-029
- [STORY-EP-031](./stories/STORY-EP-031.md) — implement — dev — 8 pts — depends_on EP-030
- [STORY-IN-030](./stories/STORY-IN-030.md) — implement — dev — 5 pts — depends_on EP-030
- [STORY-IN-031](./stories/STORY-IN-031.md) — implement — dev — 2 pts — remove Infini ToolStrip · **∥ EP-028**

## Parked (track, not this cursor)

- [STORY-EP-032](./stories/STORY-EP-032.md) — draft — `/architect` later — device UI chrome state machine
- [STORY-EP-033](./stories/STORY-EP-033.md) — ready — P0 origin/stale pen-down — **queued after EP-030**

## Carry-over candidates (not committed)

- epaper `[REQ-08]` any-node manipulation (thickened, not built)
- CHL-0011 nested enclose
- CHL-0012 FREE_FORM / align-content
- ADR-0019 amend (CHL-0018)

## Risks

- Default-on recognizers (D22) before the EXP-0002 guard corpus (ship gate, not lock gate)
- Dispatch fall-through (D21) regressing EP-016 enclose / EP-017 membership
- ToolChip rebase regressing EP-023 / EP-025 chrome

## Links to product docs

- [REQ-09](../../.docs/modules/epaper/prd.md#device-connectors) · [REQ-03](../../.docs/modules/epaper/prd.md#tool-modes)
- [connector-ink](../../.docs/modules/epaper/features/connector-ink/) · [tool-modes](../../.docs/modules/epaper/features/tool-modes/)
- [ADR-0020](../../.docs/adr/ADR-0020-connector-ink-geometry.md) · [ADR-0021](../../.docs/adr/ADR-0021-connector-toolchip.md) · [ADR-0022](../../.docs/adr/ADR-0022-recognizer-dispatch.md)
- [BS-0001](./brainstorms/BS-0001-auto-connector-ink.md) · [EXP-0002](./explorations/EXP-0002-connector-ink-warp.md)

## Execution board

- [execution-board](./execution-board.md)
