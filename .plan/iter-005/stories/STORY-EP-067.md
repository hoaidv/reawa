---
id: STORY-EP-067
title: Singleton generateNodeId for all tree nodes
kind: implement
parent_srs: [SRS-EP-07, SRS-EP-08, SRS-EP-55, SRS-IN-09]
parent_req: [REQ-04, REQ-07, REQ-11]
status: done
priority: P0
iter: iter-005
estimate: 3
owner: dev
depends_on: []
acceptance_criteria:
  - "Given any device-authored node (Ink, remnant Ink, SmartGroup, Connector, and later kinds), When the node is inserted, Then its id comes from one DeviceDocument generateNodeId (or equivalent session singleton) and never from a private counter or string concat in the caller."
  - "Given a tree that already contains s-3 and s-3_r1, When a second split of s-3 needs an extra remnant, Then generateNodeId returns an unused id (not duplicate_id) and commit applies."
  - "Given two callers in one gesture (for example remnant append plus connector create), When both mint ids, Then those ids are distinct from each other and from every id already in the tree."
  - "Given Infini applying an inbound doc_change, When the payload carries node ids, Then Infini uses those ids and does not mint a second id for the same node."
  - "Given the published op stream, When Infini and Epaper both hold the tree, Then every node id matches across the pair (0 silent remaps)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-067 — Singleton generateNodeId for all tree nodes

One allocator for every device-authored tree id. Human asked for this after brush erase committed `duplicate_id:s-3_r1` when remnant extras reused `{id}_r1`. Stopgap `allocEraseRemnantId` in `epaper/document/erase_commit.hpp` must fold into this singleton — do not keep a second remnant-only namer.

No user-interface. Human is Quality Assurance Engineer this wave: host tests + human confirm. No behavior-driven ceremony.

Canonical: [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document) unique tree; [SRS-EP-08](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-08-one-way-sync) publish; [SRS-IN-09](../../../.docs/modules/infini/features/vector-document/srs-data.md#srs-in-09-persistence-and-transmit-schemas) transmit ids; remnant extras [SRS-EP-55](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-55-clip-remnants). Ownership: [ADR-0014](../../../.docs/adr/ADR-0014-document-ownership-inversion.md).

Known private mint sites to retire (not exhaustive — grep before claiming done):

| Today | Where |
|---|---|
| `s-` + `strokeSeq` | `epaper/drawing/stroke_capture.hpp` |
| `{ink.id}_rN` skip-taken | `epaper/document/erase_commit.hpp` `allocEraseRemnantId` |
| `conn_` + stroke id | `epaper/document/recognize_connector.hpp` |
| `sg_sel_` + winner | `epaper/document/surround_create.hpp` |

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — |

## Done when

- One `generateNodeId` (name may match existing house style) owned by the document/session, used by ink ingest, erase remnants, enclose, connectors, and any other device insert
- Collision with the live tree and with ids reserved in the current gesture is impossible by construction
- Infini mirror keeps device ids on apply; no local remint
- Host test covers second remnant split after `s-3_r1` already exists (extends `test_second_split_skips_taken_remnant_id`)
- **Human verified 2026-08-31** on device
