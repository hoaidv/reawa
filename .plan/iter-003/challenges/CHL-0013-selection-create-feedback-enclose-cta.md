---
id: CHL-0013
title: Selection-create needs rubber-band feedback + Enclose CTA
author: sm
target: [REQ-05, REQ-06, SRS-EP-10, SRS-EP-11, SRS-EP-12, CHL-0010, STORY-EP-018]
severity: high
status: resolved
resolution: adopted
resolved_by: pm
resolved: 2026-08-13
opened: 2026-08-13
iter: iter-003
expedite: false
interrupts_track: ""
raised_by: human
source: human product intent (EP-018 selection feedback)
---

# CHL-0013 — Selection-create is a visible selection flow, not silent invent

## Context

[CHL-0010](./CHL-0010-undo-vs-selection-create-chrome.md) deferred `cta.create_smart_group` and
froze [STORY-EP-018](../stories/STORY-EP-018.md). Human (2026-08-13) reopened selection-create as a
**serious** feature: do **not** recognize pen events and create an ink-box with no visible feedback.

Current enclose-with-Ink-box ([STORY-EP-016](../stories/STORY-EP-016.md)) stays shipped. This challenge
is about the **Selection-tool** path only.

## Proposal (human)

1. **Rubber-band while selecting** — Pen down then move (Selection tool): a thin dotted
   selection-boundary follows the pen tip.
2. **Selection rect on pen-up** — Thin dotted selection rectangle appears around the selected
   **document nodes** (not ink-only), with **6 square anchors** (anchor events later).
3. **Explicit Enclose CTA** — User touches an **Enclose** button to create an ink-box for those
   selected nodes (surround rules from [SRS-EP-10](../../../.docs/modules/epaper/features/ink-box/srs-logic.md)
   selection-create).

## Resolution

**Adopted** — 2026-08-13 (PM). Human directed `/sm` to drive EP-018; intent is binding.

| Decision | Choice |
|---|---|
| Scope | **This campaign** (TRACK-003), design → implement after W10 |
| Create path A | Enclose-with-Ink-box (**kept**) |
| Create path B | Selection rubber-band → Enclose CTA (**adopted**) |
| ToolChip | Stays **three** tools — Selection · Pen · Ink-box. **No** fourth chip |
| Enclose control | `cta.enclose` / `cta.create_smart_group` lives on **SelectionOverlay** when a selection exists (selection-contextual), not on the chip |
| Undo chrome | Still **deferred** (CHL-0010) |
| Nesting | Still **future** (CHL-0011) — Enclose refuses if a SmartGroup is in the selection |

Architect thickens SRS-EP-10/11/12 + ADR for CTA placement. Designer extends
`device-selection-chrome` (or successor slug). SM slices a design story; EP-018 stays `draft`
until that design story is `done`.

## Product doc updates

- PRD REQ-05 creation B + Open Questions
- SRS-EP-10/11/12 (architect)
- ADR-0016 (architect) — Enclose CTA on SelectionOverlay
- CHL-0010 selection-create row superseded for invocation only (undo stays deferred)

## Interrupt / expedite

Not expedite. Does not reopen EP-017. Schedules as **W11a** (design) then EP-018 implement.
