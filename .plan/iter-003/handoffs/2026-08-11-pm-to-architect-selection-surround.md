---
from: pm
to: architect
iter: iter-003
date: 2026-08-11
subject: selection-surround-and-per-ink-fixedInk
cc: [sm, designer, qa, dev]
verdict: READY-WITH-CONCERNS
---

# PM → Architect — selection surround guard + per-ink `fixedInk` offsets

Two more UX rules on the ink-box pilot. Docs updated; SM should add/adjust stories when slicing.

## 1. Selection create (Solution 3) requires a surround stroke

Among the selected inks, **one stroke must surround almost all the others** (≥80% of each other
stroke’s samples inside that stroke’s region). The surround may be **open** — Infini builds an
**artificial closed path for the containment test only** (do not mutate stored samples). That
stroke becomes `role: boundary`; others become `content`. **If none qualify → refuse create**
(no AABB-only / hint-only Smart Group).

Recorded: [REQ-04](../../../.docs/modules/infini/prd.md#smart-group) · BR-09j ·
[SRS-IN-16](../../../.docs/modules/infini/features/vector-document/srs-logic.md#srs-in-16-selection-create-surround) ·
[ADR-0011 §4B](../../../.docs/adr/ADR-0011-smart-group.md).

## 2. `fixedInk` = per-content-ink offset (your suggested model, adopted)

Track **each** content ink’s own UV / `layoutOffset` inside the Smart Group. On `fixedInk`
resize, adjust each ink independently. New draw-into membership seeds **only** the new ink’s
offset — older content never moves. Prefer this over a shared group centroid.

Schema note in [srs-data](../../../.docs/modules/infini/features/vector-document/srs-data.md)
(`children[].layoutOffset`). Exact representation (UV vs local offset vector) is yours — one
stable round-trip field.

## Ask

Confirm SRS-IN-16 containment (artificial close) and the `layoutOffset` field shape, then SM can
slice. Designer needs a “create refused — no surround” state for the selection CTA.
