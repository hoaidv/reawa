---
from: pm
to: architect
date: 2026-09-05
iter: iter-005
---

# Hand-off: Product Manager → Architect

## Context

Human 2026-09-05: nested ink-boxes **this campaign**. Adopted
[CHL-0032](../challenges/CHL-0032-nested-ink-box.md). Schedules parked
[CHL-0011](../../iter-003/challenges/CHL-0011-nested-smartgroup-enclose.md).

Defects: (1) paste box-into-box paints but is not selectable/movable/resizable;
(2) a letter recognized as an empty ink-box is skipped by enclose and left behind.

Product: [REQ-05](../../../.docs/modules/epaper/prd.md#device-ink-box) /
[REQ-06](../../../.docs/modules/epaper/prd.md#device-manipulation) (PRD `0.17.0-draft`);
depth [BR-B20…BR-B25](../../../.docs/modules/epaper/features/ink-box/srs-product.md);
journeys `journey.device_nested_tap` / `journey.device_enclose_flatten`.
Clipboard paste-into-box stays [SRS-EP-73](../../../.docs/modules/epaper/features/clipboard/srs-product.md)
and must leave a nested child **tap-selectable**.

## Review verdict

**READY-WITH-CONCERNS**

| Class | Finding |
|---|---|
| Strength | Outcome REQs unchanged; five rules are testable; flatten kills the letter leftover; tap-vs-marquee split keeps existing lasso UX |
| Strength | `prd-check`: **0 FAIL** (21 WARN, all pre-existing open-question / Needs-design parser noise) |
| Concern | “Natural area” and `RenderingContext` are **algorithm** — Architect binds; PM stated 80% + highest paint-order only |
| Concern | Rotation **UI** still out (REQ-08); Rules 1–3 may bake a reserved rotation field |
| Gap | none blocking bind |

`prd-check` run with this handoff.

## Asks

1. Bind BR-B20…B25 into SRS-EP-10 / SRS-EP-11 (and new sections if cleaner). Mint IDs as needed.
2. Domain: SmartGroup children = Ink **or** nested SmartGroup; empty = boundary-only.
3. ADR for RenderingContext compose vs baking world samples (costly to reverse).
4. Retire “SmartGroup in selection → refuse” and “free top-level Ink only” in SRS-EP-10 **in place** (`lifecycle` notes, `superseded-by` the new nested sections).
5. Quality rows: nested tap after camera change; flatten; reparent 80%; marquee top-level.

## Constraints

- **Needs design: no** this slice — same overlay / toolbar inventory. Nested tap reuses `sel.selected`.
- Human is QA; BDD before implement is still expected on the new AC.
- Do not reopen CHL-0012 (`FREE_FORM` / `WRAP_CONTENT`).
- Execution lock already includes `epaper/ink-box`. Stop line `verified`.

## Out of scope

- Marquee/freeform independent pick of nested children.
- Rotation chrome (REQ-08).
- Infini authoring of nested boxes (device authors; Infini mirrors).
- Field follow-ups EP-070…072 (unchanged).
