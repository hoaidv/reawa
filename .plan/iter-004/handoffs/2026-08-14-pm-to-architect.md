---
from: pm
to: architect
date: 2026-08-14
iter: iter-004
---

# Hand-off: PM → Architect — REQ-09 + ToolChip 3+2

## Context

[BS-0001](../brainstorms/BS-0001-auto-connector-ink.md) is **concluded**. Open questions
(blink waveform, inflection cutoff, live-drag panel rate) go to follow-up stories / EXP-0002
Initiative 2, not more brainstorm rounds.

PRD [epaper/prd.md](../../../.docs/modules/epaper/prd.md) **v0.6.0**:

- **[REQ-09]** On-device connectors — Must, above REQ-08 / CHL-0011 / CHL-0012, `needs_design: yes`
- **[REQ-03]** revised — 3 exclusive tools + 2 recognizer toggles + Undo/Redo
- **[REQ-05]** enclose is `Pen` + Ink-box recognition armed
- **[REQ-06]** connector attachment is REQ-09 (rotation still REQ-08)
- Non-Goals: "ordinary ink forever in Pen" retired; toolbar is 3+2+Undo/Redo

Default routing is **not a picker**: auto-pick from the rest spine → **Ink** (`morph`) or
**Curve** (`cubic`). Creator may change style after create.

## Review verdict

**READY-WITH-CONCERNS**

| Class | Finding |
|---|---|
| Strength | REQ-09 acceptance is measurable (visible p95, 0 peer messages, one undo, live ≥5 Hz, 0 divergent nodes, ≤2% FP ship gate) |
| Strength | ToolChip inventory is a closed binding string, not a mood |
| Concern | `srs-ui` for ToolChip was still listing `tool.ink_box` at handoff time — architect must finish SRS-EP-05 inventory before designer paints |
| Concern | Guard corpus (EXP-0002 Initiative 2) is a **ship** gate, not a lock gate |
| Gap | none that block decomposition — ADRs already drafted in the same session |

`prd-check` / `gate` run with the architect close (same day).

## Asks

1. Finish SRS-EP-05 closed inventory (3 tools + 2 toggles; retire `tool.ink_box` states).
2. Keep ADR-0020/21/22 as the decision record; do not reopen Ink vs Curve at draw time.
3. Hand SM a sliceable SRS set (recognition, warp, chrome, quality).

## Constraints

- Do not start REQ-08 / CHL-0011 / CHL-0012.
- Do not copy EXP-0002 probe code into production SRS as algorithm-by-paste; cite numeric results only.
- Recognizers **ship armed**; false-positive bar ≤2% is the ship gate.

## Out of scope

- Arrowheads, dash/double, squared routing, desktop connector authoring, physics rope.
