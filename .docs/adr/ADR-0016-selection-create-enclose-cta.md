---
id: ADR-0016
title: Selection-create Enclose CTA on SelectionOverlay
status: accepted
amended_by: [ADR-0017]
date: 2026-08-13
deciders: [architect, pm]
supersedes: null
amends: [ADR-0013]
source: CHL-0013
---

# ADR-0016 — Selection-create Enclose CTA on SelectionOverlay

## Context

[REQ-05](../modules/epaper/prd.md#device-ink-box) Creation B needs an on-panel way to invoke
Smart Group from a selection. [CHL-0010](../../.plan/iter-003/challenges/CHL-0010-undo-vs-selection-create-chrome.md)
deferred `cta.create_smart_group` because the ToolChip is a closed three-tool inventory
(Selection · Pen · Ink-box) and a fourth chip was rejected.

[CHL-0013](../../.plan/iter-003/challenges/CHL-0013-selection-create-feedback-enclose-cta.md)
adopts selection-create with visible feedback: rubber-band → selection rect + 6 anchors →
explicit **Enclose** control. The placement of that control must not invent a fourth tool.

Quality goals: discoverable create path; 0 silent invent; compose with [ADR-0013](./ADR-0013-ink-box-tool-modes.md)
tool arming; keep finger-only chip hit targets.

## Decision

1. **ToolChip stays three tools.** No fourth ToolChip slot for Enclose or undo.
2. **`cta.enclose`** (alias of creation-B invoke; same as `cta.create_smart_group` in logic) lives on
   **SelectionOverlay**, visible only when `selection` tool is armed **and** the selection set is
   non-empty. Finger or pen may activate it.
3. **Ink-box tool enclose** (Creation A) remains a separate path — unchanged.
4. **Rubber-band + 6 anchors** are SelectionOverlay chrome for multi-node select; they are not a
   ghost of pending ink-box geometry. Create still requires surround rules ([SRS-EP-10](../modules/epaper/features/ink-box/srs-logic.md)).

## Consequences

- Designer must extend selection chrome states (`sel.marquee`, `sel.nodes_selected`, Enclose CTA,
  refuse) — [STORY-EP-022](../../.plan/iter-003/stories/STORY-EP-022.md).
- Implement EP-018 depends on EP-022 design `done`.
- Undo chrome remains deferred (CHL-0010).
- Risk: overlay clutter near dense ink — Designer owns 1-bit legibility (SRS-EP-12 open row).

## Alternatives Considered

| Option | Why rejected |
|---|---|
| Fourth ToolChip for Enclose | Contradicts SRS-EP-05 closed inventory; CHL-0010 |
| Pen gesture with no CTA | Human forbids silent invent; refuse path needs an explicit invoke |
| Defer Creation B entirely | Rejected by CHL-0013 Adopt — human driving EP-018 this campaign |
| Replace Ink-box tool with selection-only create | Loses working Creation A (EP-016 shipped) |
