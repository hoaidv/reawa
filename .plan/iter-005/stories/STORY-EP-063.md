---
id: STORY-EP-063
title: Geometric clip, remnant split, boundary polyline
kind: implement
parent_srs: [SRS-EP-55, SRS-EP-59]
parent_req: [REQ-11]
status: done
priority: P0
iter: iter-005
estimate: 8
owner: dev
depends_on: []
acceptance_criteria:
  - "Given an Ink polyline and a clip region, When clip runs, Then leftover geometry is polyline intersect region (not sample-in-region drop) and 0 chords span a hole."
  - "Given two remnants after clip, When commit, Then the longest keeps the original id (set_ink_samples); extras are append_ink siblings (same parent, style, adjacent paint order); one compound if more than one op; never restore_snapshot."
  - "Given remnants shorter than 1 mm world, When commit, Then those remnants are discarded."
  - "Given 0 remnants, When commit, Then remove_node that Ink."
  - "Given SmartGroup create, When the group exists, Then an invisible closed boundary polyline is seeded from the enclose, persisted, and transforms with boundary ink."
  - "Given brush/area across visible boundary ink, When clip commits, Then boundary ink may split (role: boundary; broken surround allowed); the boundary polyline is unchanged."
  - "Given brush/area that removes all visible ink of a SmartGroup, When commit, Then the SmartGroup is removed (polyline does not keep an empty box)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-063 — Geometric clip, remnant split, boundary polyline

Shared **engine** for brush and area. No ToolChip, no object-erase 80% table. Host-testable without HID.

Canonical: [prd-erase.md](../../../.docs/modules/epaper/prd-erase.md) §§10–11. Bind: [SRS-EP-55](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-55-clip-remnants). Decision: [ADR-0034](../../../.docs/adr/ADR-0034-erase-clip-remnants.md). Domain: [vector-document SmartGroup](../../../.docs/domain/vector-document.md). Undo ops already exist: `set_ink_samples`, `append_ink`, `remove_node`, `compound` ([ADR-0032](../../../.docs/adr/ADR-0032-inverse-op-undo.md)).

Replace any leftover Path A `commitPathAErase` sample-delete / concatenate-leftovers (chord) with this engine. Seed boundary polyline on **new** SmartGroups; existing groups without one need a documented create-time seed (do not invent a synthetic rectangle).

Human is QA this wave: host tests + human confirm. No BDD ceremony required before implement.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — (can land in parallel with [STORY-EP-062](./STORY-EP-062.md)) |

## Done when

- Clip is geometry; remnant split + 1 mm floor; 0 chords
- Boundary polyline persisted, never clipped
- Last visible ink gone → SmartGroup `remove_node`
- One undo restores when wired through a caller (brush/area stories)
- **Human verified 2026-08-29**: remnant split on brush; `duplicate_id:{id}_r1` on second nick fixed via skip-taken remnant ids (fold into [STORY-EP-067](./STORY-EP-067.md))
