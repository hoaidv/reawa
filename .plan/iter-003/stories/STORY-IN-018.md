---
id: STORY-IN-018
title: "Tool intent transport (stroke intent, pickables, tool_intent)"
kind: implement
parent_srs: [SRS-IN-13]
parent_req: [REQ-04]
status: done
priority: P1
iter: iter-003
estimate: 5
owner: dev
depends_on: [STORY-IN-010, STORY-IN-015]
acceptance_criteria:
  - "Given Epaper enclose stroke, When stroke_begin is sent, Then intent enclose|ink rides the stroke (default-safe additive)."
  - "Given Infini SmartGroups, When doc_snapshot is published, Then pickables[] lists world bounds for device local pick."
  - "Given Epaper Selection drag on a pickable, When the gesture completes, Then a narrow tool_intent message updates Infini without opening bidirectional doc_op."
  - "Given tool mode changes on either device, When observed on the wire, Then mode itself is never synced (device-local UI state)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-018 — Tool intent transport

Implements [SRS-IN-13](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md)
per ADR-0013. **No design** (protocol).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | IN-010, IN-015 |

## Done when

- AC green `@SRS-IN-13`
