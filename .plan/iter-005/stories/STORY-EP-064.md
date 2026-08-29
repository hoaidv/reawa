---
id: STORY-EP-064
title: Brush erase capsule clip
kind: implement
parent_srs: [SRS-EP-56, SRS-EP-59]
parent_req: [REQ-11]
status: in-review
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-062, STORY-EP-063]
acceptance_criteria:
  - "Given erase_brush and world ink, When Primary draws a capsule across a stroke, Then intersecting geometry is gone, remnants >=1 mm are separate Ink nodes (longest keeps original id), p95 <=50 ms after up, and one undo restores the pre-erase tree (+-1 px @ 100% zoom; skip/no-op per REQ-04)."
  - "Given zoom-in so ink looks larger, When the hover circle and ghost are shown, Then they scale with the same world diameter (8 mm), not a constant panel millimetre."
  - "Given pen near and erase_brush, When the pen is in proximity (enter or up to near), Then a white-filled circle (0.5 mm black stroke, 8 mm diameter) follows the tip and 0 samples are deleted until down. Kill-switch for field test."
  - "Given a connector or Frame under the capsule, When the gesture commits, Then those nodes are unchanged (Primitive and Text also no-op)."
  - "Given brush ghost in progress, When pointer-up commits, Then the ghost is dropped in the same refresh as document damage."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-064 — Brush erase capsule clip

`BrushErase` Operation on the `Eraser` mode from [STORY-EP-062](./STORY-EP-062.md). Capsule radius **4 mm** world; hover diameter **8 mm**. Clip via [STORY-EP-063](./STORY-EP-063.md).

Canonical: [prd-erase.md](../../../.docs/modules/epaper/prd-erase.md) §7, §14 Brush. Bind: [SRS-EP-56](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-56-brush). Units: 1 mm ≈ 8.90 du @ 226 dpi ([erase SRS](../../../.docs/modules/epaper/features/erase/srs-logic.md)). Preview on ToolCanvas only.

Nib (if reported) uses this mutation; chip already shows `erase_brush` in EP-062.

Human is QA this wave: host tests + human confirm. No BDD ceremony required before implement.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-062, STORY-EP-063 |

## Done when

- Ink clipped; other kinds no-op; Frame/connector 0 mutations
- Hover + ghost world millimetres; ghost teardown with damage
- p95 ≤50 ms after up; 0 chords; one undo
