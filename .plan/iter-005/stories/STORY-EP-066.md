---
id: STORY-EP-066
title: Object erase 80 percent table
kind: implement
parent_srs: [SRS-EP-58, SRS-EP-59]
parent_req: [REQ-11]
status: draft
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-062]
acceptance_criteria:
  - "Given erase_object and a lasso containing >=80% arc length of an Ink, When commit, Then that Ink is gone (0 remnant nodes for that id) and one undo restores it."
  - "Given a SmartGroup whose boundary-polyline area is >=80% inside the lasso, When commit, Then the whole group is gone."
  - "Given a SmartGroup whose area is <80% inside, When commit, Then the group and children are unchanged (even if some child ink is inside the lasso)."
  - "Given a Connector with >=80% of warped V length inside, When commit, Then the connector is removed, each attachment is a world node at last pose (unbound, parent = former connector parent), and one undo restores connector + binds."
  - "Given a Frame >=80% inside the lasso, When commit, Then the Frame is still there."
  - "Given object dotted freeform, When candidates pass 80%, Then ToolCanvas shows AABB highlight (thicker); never restroke document ink; chrome drops with document damage; p95 <=50 ms after up."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-066 — Object erase 80 percent table

`ObjectErase` Operation. Whole nodes only — **0 remnants**. Does **not** require the clip engine ([STORY-EP-063](./STORY-EP-063.md)). Needs the mode/chip from [STORY-EP-062](./STORY-EP-062.md). Boundary polyline for SmartGroup area must exist (seeded in EP-063; if EP-066 lands first, seed polyline in this story for create path and keep EP-063 as persist/clip owner).

80% table ([prd-erase.md](../../../.docs/modules/epaper/prd-erase.md) §9.2 / [SRS-EP-58](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-58-object)):

| Kind | Measure |
|---|---|
| Ink | arc length |
| SmartGroup | polygon **area** of **boundary polyline** |
| Primitive, Text | world AABB area |
| Connector | warped `V` length |
| Frame | never |

Connector remove (this story **or** area fully-inside): unbind attachments; reparent to connector’s parent; last derived world pose; adjacent paint order; undo rebinds. **0** convert-to-Ink. REQ-13 endpoint-ink out of this wave.

Path B “erase selected nodes” stays cancelled ([STORY-EP-042](./STORY-EP-042.md)). Cut remains.

Human is QA this wave: host tests + human confirm. No BDD ceremony required before implement.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-062 |

## Done when

- 80% table honored; Frame 0; SmartGroup all-or-nothing
- Connector unbind + undo rebind
- AABB highlight on ToolCanvas only
