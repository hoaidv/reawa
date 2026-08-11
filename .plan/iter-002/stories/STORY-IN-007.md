---
id: STORY-IN-007
title: "In-memory document tree and idempotent op apply"
kind: implement
parent_srs: [SRS-IN-04]
parent_req: [REQ-02]
status: done
priority: P1
iter: iter-002
estimate: 5
owner: dev
depends_on: []
acceptance_criteria:
  - "Given an empty Infini document, When ops create Frame/Ink/Text/Primitive/Group/Connector per SRS-IN-04, Then the materialised tree matches invariants (unique ids, Frame root-only, Group nesting rules)."
  - "Given a valid op with opId, When the same opId is applied twice, Then the tree is unchanged on the second apply (idempotent)."
  - "Given a connector with port or boundary anchors, When an endpoint node moves, Then anchors re-resolve to the live boundary."
  - "Given a materialised tree, When flattenDrawables runs, Then WorldLayer can cull/paint ink and leaves (SmartGroup transform applied when present)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-007 — In-memory document tree and idempotent op apply

Implements [SRS-IN-04](../../../.docs/modules/infini/features/vector-document/srs-logic.md)
per [ADR-0010](../../../.docs/adr/ADR-0010-tree-of-vectors.md). **No design story** (DocChrome
cancelled). Wave **W4** foundation — start here under `wip: 1`.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — |

## Done when

- AC green under BDD `@SRS-IN-04`
- Sync-Auditor: no orphan for SRS-IN-04 on done
