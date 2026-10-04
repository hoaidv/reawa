---
id: CHL-0033
author: pm
target: [REQ-04, REQ-06, REQ-08, SRS-EP-07, SRS-EP-09, SRS-EP-10, SRS-EP-11, SRS-EP-12, SRS-EP-14, SRS-EP-21, SRS-EP-75, SRS-EP-76, SRS-EP-77, SRS-EP-78, SRS-EP-79]
severity: high
status: adopted
opened: 2026-10-04
iter: iter-006
expedite: false
interrupts_track: ""
resolution: adopted
resolved_by: pm
resolved: 2026-10-04
raised_by: human
source: ADR-0041 accepted; step 0.1 of the document-forest plan
---

# CHL-0033 — Epaper product records under the document forest

## Context

The human accepted [ADR-0041](../../../.docs/adr/ADR-0041-document-forest.md) (Document forest,
concurrent readers, progressive paint) for the rebuilt `epaper/` on 2026-10-04. Several Epaper
records describe the archived tree (`epaper_old/`): the `inkScaleMode` toggle, resize through a
transform, own-transform nested paint, and one global hit-test index rebuilt per commit.

The rebuilt Epaper has only the pen so far. The ink-box, selection, touch and manipulation tools
come later, each on top of the forest. The forest plan
([document-forest-implementation.md](../../document-forest-implementation.md)) builds the engine,
and only its geometry queries are product behavior today.

## Proposal

Human decision, 2026-10-04: replace the two hit-test records now; deprecate the ink-box records
until each tool is ported; keep REQ-08 parked.

## Resolution

**Adopted** — 2026-10-04 (PM, human decision).

| Record | Disposition | Reason |
|---|---|---|
| [SRS-EP-79](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-79-geometry-queries) | **retired** → [SRS-EP-80](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-80-forest-geometry-queries) | The plan builds the queries now. Same named queries and product rules; one index per container instead of one global index |
| [SRS-EP-78](../../../.docs/modules/epaper/features/device-document/srs-quality.md#srs-ep-78-log-hit-test) | **retired** → [SRS-EP-81](../../../.docs/modules/epaper/features/device-document/srs-quality.md#srs-ep-81-forest-query-quality) | Same product bars; "rebuild per commit" rows become "update per commit"; probe bound per container |
| [REQ-06](../../../.docs/modules/epaper/prd.md#device-manipulation) | **deprecated** | `manipMode` replaces `inkScaleMode`; resize writes `bounds`. Successor REQ when ink-box manipulation is ported |
| [SRS-EP-10](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-10-device-recognition), [-11](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-11-device-manipulation), [-21](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-21-one-finger), [-75](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-75-nested-membership), [-76](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-76-nested-render), [-77](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-77-nested-hit-reparent), [-12](../../../.docs/modules/epaper/features/ink-box/srs-ui.md#srs-ep-12-selection-chrome), [-14](../../../.docs/modules/epaper/features/ink-box/srs-quality.md#srs-ep-14-ink-box-quality) | **deprecated** | Each record's note states the successor delta. Successors are written when the behavior is ported, before its story is `ready` |
| [REQ-08](../../../.docs/modules/epaper/prd.md#node-manipulation) | **not affected**, still parked | ADR-0041 decides the move and resize model per kind; no product verb is activated |
| [REQ-04](../../../.docs/modules/epaper/prd.md#device-document), [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document) | **active**, change note added | Obligations unchanged. Storage in child space and the per-container index are design; `doc_load` and shared semantics wait on [CHL-0034](./CHL-0034-forest-wire-sync-deferred.md) |
| [SRS-EP-09](../../../.docs/modules/epaper/features/device-document/srs-data.md#srs-ep-09-device-data) | **not affected** | Wire binding; deferred with Infini sync ([CHL-0034](./CHL-0034-forest-wire-sync-deferred.md)) |
| [SRS-EP-18](../../../.docs/modules/epaper/features/connector-ink/srs-logic.md) (connector warp) | **not affected** | The forest's connector rules (face-frame departure, centre-end ray) match shipped behavior |

## Product doc updates

Done 2026-10-04:
- lifecycle markers and notes on every record above;
- SRS-EP-80 and SRS-EP-81 written;
- [ADR-0041 consequences](../../../.docs/adr/ADR-0041-document-forest.md#consequences) updated.

Pending (owner: PM, with the architect): a successor REQ or SRS for each deprecated record, when its
tool is ported. CHL-0022, CHL-0023 and CHL-0027 (deferred) feed those successors.

## Consequences and verification

- Stories of [TRACK-009](../../tracks/TRACK-009-document-forest.md) trace to REQ-04, SRS-EP-07, -80,
  -81, -01, -02, -13 and -18. None traces to a deprecated record.
- Cancelled: STORY-EP-078…080 (implemented ADR-0040 on the archived tree).
- Verification: `adlc validate --check` shows no new violations from these edits (2026-10-04).
