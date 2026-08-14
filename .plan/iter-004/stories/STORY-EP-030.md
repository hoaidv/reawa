---
id: STORY-EP-030
title: Recognize single-stroke and chained connectors
kind: implement
parent_srs: [SRS-EP-17]
parent_req: [REQ-09]
status: draft
priority: P0
iter: iter-004
estimate: 5
owner: dev
depends_on: [STORY-EP-027, STORY-EP-029]
acceptance_criteria:
  - "Given two SmartGroups and recog.connector armed, When the creator draws an open stroke A→C, Then create_connector commits with body, rest shape, anchors, warpStyle; visible p95 ≤500 ms; 0 peer messages; one undo restores ink (SRS-EP-17)."
  - "Given UX2 three strokes, When the last lands on C, Then one connector, one op, one undo; style from the merged spine (D6, D40)."
  - "Given guards fail or the toggle is off, When pen-up runs, Then the stroke stays ordinary ink and conn.rejected has no banner."
  - "Given create, When chrome runs, Then ovl.conn_blink matches the EP-027 package."
design_package: ".plan/iter-004/design/connector-chrome/"
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-030 — Recognize single-stroke and chained connectors

[SRS-EP-17](../../.docs/modules/epaper/features/connector-ink/srs-logic.md).
Device authors `create_connector` ([SRS-EP-07](../../.docs/modules/epaper/features/device-document/srs-logic.md)).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-027, EP-029 |
