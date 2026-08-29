---
id: STORY-EP-065
title: Area erase clip and fully-inside remove
kind: implement
parent_srs: [SRS-EP-57, SRS-EP-59]
parent_req: [REQ-11]
status: draft
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-062, STORY-EP-063]
acceptance_criteria:
  - "Given erase_area and an open freeform across ink, When pointer-up, Then the polygon auto-closes last to first, even-odd interior ink is clipped, remnants follow remnant rules, and 0 minimum-area refusal."
  - "Given erase_area and a connector (or Primitive / Text / SmartGroup) fully inside the closed polygon, When commit, Then that node is removed (SmartGroup as a whole; connector attachments unbound per object-erase unbind rules)."
  - "Given erase_area and a connector only partially inside, When commit, Then the connector is unchanged (no clip, no convert-to-Ink)."
  - "Given a Frame fully inside the area polygon, When commit, Then the Frame is still there."
  - "Given area dotted freeform on ToolCanvas, When pointer-up, Then chrome drops in the same refresh as document damage; one undo restores; p95 <=50 ms after up."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-065 — Area erase clip and fully-inside remove

`AreaErase` Operation. Ink = geometric clip ([STORY-EP-063](./STORY-EP-063.md)). Other kinds = `remove_node` if **fully inside**. Frame never. Connector unbind = same as [STORY-EP-066](./STORY-EP-066.md) § connector remove (implement unbind here or share a helper; do not convert to Ink).

Canonical: [prd-erase.md](../../../.docs/modules/epaper/prd-erase.md) §8, §14 Area. Bind: [SRS-EP-57](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-57-area). Preview: dotted polyline on ToolCanvas; no cover fill.

Human is QA this wave: host tests + human confirm. No BDD ceremony required before implement.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-062, STORY-EP-063 |

## Done when

- Auto-close + even-odd Ink clip; fully-inside remove; Frame 0; partial connector 0
- One undo; p95 ≤50 ms; 0 chords
