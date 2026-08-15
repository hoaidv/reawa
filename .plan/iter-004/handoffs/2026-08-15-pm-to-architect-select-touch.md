---
from: pm
to: architect
date: 2026-08-15
iter: iter-004
---

# Hand-off: PM → Architect — connector select + hand-touch

## Context

Human verified through EP-029. Product gap: recognized connectors have selected chrome (Ink/Curve, Edge/Centre) but no pick grammar. Second ask: start capacitive **hand-touch** on the canvas, not only ToolChip.

PRD [epaper 0.7.0](../../../.docs/modules/epaper/prd.md):
- **[REQ-09](#device-connectors)** — added select via `sel_rect` / `sel_freeform` (≥80% **path samples**) and **pen-down on the stroke** (not AABB). BR-C11 in connector `srs-product`.
- **[REQ-10](#hand-touch)** — new. Finger hit ink-box → `sel_freeform` + select; finger **move**; **no** finger on hit targets **< 64 du** (primary ToolChip tile; 32 was too small — CHL-0019). Finger empty canvas: no tool switch, no lasso.

Prioritisation: [prioritization.md](../prioritization.md) — REQ-09 select then REQ-10.

## Review-prd

**Verdict: READY-WITH-CONCERNS**

| Class | Finding |
|---|---|
| Strength | Outcome-first: pick a connector you drew; move a box with a finger without hunting the chip. Size rule is one number (64). |
| Strength | MECE: select grammar on REQ-09; modality on REQ-10; REQ-06 stays pen resize. |
| Concern | REQ-10 needs_design; existing EP-027 chrome may cover connector-selected; finger journeys need a design story. Dual-ask QA + designer. |
| Concern | Palm rejection / finger vs pen routing is architect (device input). Do not invent a second pointer model in SRS without an ADR if the current event split is insufficient. |
| Gap (resolved) | 32 vs 64 — closed: 64. |

Actions: PRD + BR-C11 written. SRS logic/ui for REQ-10 and connector hit-test still yours.

## Asks
1. Decompose REQ-09 select + REQ-10 into SRS (connector-ink + ink-box / tool-modes as needed). ADR if pen vs finger routing is a non-trivial choice.
2. Keep REQ-08 out of this slice (re-anchor still later).
3. Hand SM stories: implement connector select first; design+implement hand-touch second.

## Constraints
- Vertical lock, verified stop, wip 2. Do not reopen nested enclose / FREE_FORM.
- Never touch code (PM). You own architecture only.
- Finger is **not** pan/zoom/pinch.

## Out of scope
- Finger resize, rotation, connector re-anchor.
- Implementing EP-031 (dev in flight).

## Next
User: **`/architect`** then **`/sm`**.
