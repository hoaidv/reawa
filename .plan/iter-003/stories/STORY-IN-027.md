---
id: STORY-IN-027
title: "Desktop doc_change applier"
kind: implement
parent_srs: [SRS-IN-07, SRS-IN-08]
parent_req: [REQ-03]
status: done
priority: P0
iter: iter-003
estimate: 5
owner: dev
depends_on: []
acceptance_criteria:
  - "Given an inbound doc_change carrying append_ink (or create_smart_group / set_smart_group_transform / set_ink_scale_mode / reparent / remove / restore_snapshot), When it is applied, Then the op lands in VectorDocument and WorldLayer paints from the mirror — not from rebuildWithRmInk flat primitives."
  - "Given the same opId applied twice, When the second arrives, Then the tree is unchanged (idempotent)."
  - "Given last applied seq 12, When a doc_change arrives with baseSeq 15, Then the mirror is marked suspect, an explicit resync is requested, and a suspect mirror is not saved."
  - "Given a preview path for a stroke, When the matching doc_change applies, Then the preview is removed and the mirror node paints in its place (0 preview paths persisted or saved)."
  - "Given a live session, When Infini is used as a viewer, Then Infini emits 0 document ops; the only outbound document message is the epoch's doc_load (IN-028)."
  - "Given device commit → mirror apply, When measured, Then p95 ≤300 ms (SRS-IN-08)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-027 — Desktop doc_change applier

Implements the inbound applier in [SRS-IN-07](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md)
(architect ask 4: **the whole desktop change**). Shipped `rebuildWithRmInk` writes flat primitives
and nothing enters `VectorDocument` — that path is withdrawn.

BDD: [session-channels.feature](../../../.docs/modules/infini/features/tablet-sync/bdd/session-channels.feature)
(`@SRS-IN-07` apply / gap / preview-replace / `@future` applier scenarios). Quality: [SRS-IN-08](../../../.docs/modules/infini/features/tablet-sync/srs-quality.md).
QA walk: [2026-08-13-qa-to-dev-w9.md](../handoffs/2026-08-13-qa-to-dev-w9.md) — treat IN-027 AC
`set_smart_group_transform` / `remove` as SRS-IN-09 `set_smart_transform` / `remove_node`.

No design (protocol + mirror). May run in W9 ∥ device document once EP-013 has passed — does
not wait on device publish, the wire contract is ADR-0015.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — |

## Done when

- Applier scenarios green; WorldLayer source is the mirror tree
- `@implements [SRS-IN-07]` on the applier
