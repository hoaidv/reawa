---
id: CHL-0022
author: architect
target: [SRS-EP-04, SRS-EP-11, SRS-EP-19, ADR-0009]
severity: medium
status: open
opened: 2026-08-19
iter: iter-005
expedite: false
interrupts_track: ""
---

# CHL-0022 — Shipped “no device pan / no arrowheads” prose vs TRACK-005

## Context

W0 bound [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) and [REQ-13](../../../.docs/modules/epaper/prd.md#connector-ends) as **new** SRS ids. Shipped sections still contain clauses that would contradict those ids if implemented as written:

| Shipped text | Conflict |
|---|---|
| [SRS-EP-04](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md) Input routing: “Finger on canvas \| Ignored (no on-device pan)” | One-finger pick/move + two-finger pan are [SRS-EP-21](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-21-one-finger) / [SRS-EP-24](../../../.docs/modules/epaper/features/region-sync/srs-logic.md#srs-ep-24-two-finger-viewport) |
| [SRS-EP-11](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-11-device-manipulation) “not to pan, because there is no on-device pan” | Same — annotated 2026-08-19; parent of REQ-10 is **not** SRS-EP-11 |
| [SRS-EP-19](../../../.docs/modules/epaper/features/connector-ink/srs-ui.md) “Out of scope: Arrowheads…” | Endpoint styles are [SRS-EP-36](../../../.docs/modules/epaper/features/connector-ink/srs-ui.md#srs-ep-36-endpoint-toolbar) |
| [ADR-0009](../../../.docs/adr/ADR-0009-shared-document-viewport.md) “Infini owns translate + uniform scale” | Amended by [ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md) (proposed) — do not overwrite ADR-0009 |

Architect did **not** silently rewrite those shipped sections’ behavioral tables. New ids are the parents for TRACK-005 stories.

## Proposal

PM adopt this challenge:

1. Amend SRS-EP-04 finger-on-canvas row to **link** SRS-EP-21 / SRS-EP-24 (keep SRS-EP-04 as REQ-03 parent).
2. Keep SRS-EP-11 as REQ-06; leave the 2026-08-19 pointer.
3. Amend SRS-EP-19 out-of-scope to “dash/width/routing names” only.
4. Leave ADR-0009 append-only; ADR-0023 is the amendment.

## Resolution
<!-- PM fills after triage: adopted | deferred | rejected -->

## Product doc updates
<!-- List docs updated if adopted -->

## Interrupt / expedite (when applicable)
<!-- not expedite — TRACK-005 continues on new SRS ids -->
