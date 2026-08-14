---
id: ADR-0021
title: ToolChip: 3 exclusive tools, 2 recognizer toggles, Undo/Redo
status: accepted
date: 2026-08-14
deciders: [architect, pm, human]
supersedes: [ADR-0017]
amends: [ADR-0013, ADR-0016, ADR-0018]
source: BS-0001 D14 / D23 / D24
---

# ADR-0021 — ToolChip: 3 exclusive tools, 2 recognizer toggles, Undo/Redo

## Context

[ADR-0017](./ADR-0017-four-tool-chip.md) closed the primary bar at four exclusive tools:
`sel_rect` · `sel_freeform` · `pen` · `ink_box`. [ADR-0018](./ADR-0018-undo-redo-chip-actions.md)
then added Undo/Redo as **actions** after a gap, not as a fifth tool.

[BS-0001](../../.plan/iter-004/brainstorms/BS-0001-auto-connector-ink.md) D14 splits **tools**
from **recognizers**: ink-box creation is a toggle on `pen`, not an exclusive tool. A second
toggle arms connector recognition. Human layout (2026-08-14):

**Rect selection | Freeform selection | Pen ⟨space⟩ Ink-box recognizer | Connector recognizer ⟨space⟩ Undo | Redo**

[REQ-03](../modules/epaper/prd.md#tool-modes) is amended to this inventory. Enclose stays
contextual ([ADR-0016](./ADR-0016-selection-create-enclose-cta.md)). Quality: one tap to arm
a tool; toggles independent of the exclusive tool; chip still partial-refresh; [REQ-01](../modules/epaper/prd.md#local-pen-ink)
ink latency unchanged.

## Decision

1. Exclusive tools are exactly three: `sel_rect` · `sel_freeform` · `pen`. Default remains `pen`.
   There is no `tool.ink_box`.
2. Two **independent recognizer toggles** sit after a gap: `recog.ink_box` ("Ink-box recognition"),
   `recog.connector` ("Connector recognition"). They are not `toolMode`. Both **ship armed**
   (D22). While a Selection tool is active they are **dimmed** (armed state retained) (D23).
3. Tool **and** both toggle states **latch at pen-down** for the whole stroke (D15).
4. Undo and Redo remain actions after a second gap ([ADR-0018](./ADR-0018-undo-redo-chip-actions.md)).
   Enclose still must not join this row.
5. Tile size and floating chip geometry stay as shipped ([SRS-EP-05](../modules/epaper/features/tool-modes/srs-ui.md)).
   Hug-width grows by one tile + one gap versus the ADR-0018 chip.

## Consequences

- [SRS-EP-04](../modules/epaper/features/tool-modes/srs-logic.md) / [SRS-EP-05](../modules/epaper/features/tool-modes/srs-ui.md)
  closed inventory is 3 tools + 2 toggles + 2 actions. New design package required (UI-EP-01 /
  UI-EP-02 ToolChip scenes are stale).
- Pen-up dispatch is [ADR-0022](./ADR-0022-recognizer-dispatch.md), not "whichever exclusive
  tool is armed".
- [ADR-0017](./ADR-0017-four-tool-chip.md) is **superseded**. BDD that asserts "exactly four
  tools `… ink_box`" is retired with the design story.

## Alternatives Considered

| Approach | Why rejected |
|---|---|
| Keep `ink_box` as a fourth exclusive tool + add a connector tool | Fifth exclusive tool; two mode switches to draw then connect |
| Fifth exclusive connector tool, keep ink-box as tool | Same; contradicts "as natural as ink-box" |
| Recognizers only, no toggles | Violates BR-B02 "never unprompted" |
| Undo as exclusive tool | Already rejected by ADR-0018 |
