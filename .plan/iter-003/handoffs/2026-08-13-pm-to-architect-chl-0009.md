---
from: pm
to: architect
date: 2026-08-13
iter: iter-003
source: CHL-0009
---

# Hand-off: PM → Architect — CHL-0009 adopted

## Context

[CHL-0009](../challenges/CHL-0009-missing-device-document-srs-logic.md) is **adopted**.
`device-document/srs-logic.md` was claimed in the 2026-08-13 architect handoff and is
linked from product/data/quality/BDD/infini — but is not on disk. Completeness gap, not a
direction change.

## Asks

1. Author `.docs/modules/epaper/features/device-document/srs-logic.md` with exactly
   `[SRS-EP-07]` (tree, ingestion, op set, undo ring) and `[SRS-EP-08]` (inbound
   classification, load handshake, publish queue, preview).
2. Use the already-assigned ids. No new ids. No PRD rewrite. Do not rewrite ADR-0014/0015.
3. Bind to existing product (BR-D01…D12), data (`SRS-EP-09`), quality (`SRS-EP-13`), BDD
   (`undo-ring.feature`, `one-way-sync.feature`), domain, and infini `SRS-IN-07`.
4. Fill CHL-0009 `## Product doc updates` with the new file path.
5. Hand off to SM with a review verdict. SM may then flip EP-014 / EP-015 / EP-020
   `blocked` → `draft` (still wait on EP-013 before `ready`). Do not start W9 implement.

## Constraints

- No peer round trip inside an editing gesture.
- Do not invent journeys. Do not put quality numbers in logic except by reference to SRS-EP-13.
- Do not fork the wire grammar (canonical in infini SRS-IN-09).
- Do not edit `epaper/` code, design packages, or story statuses.

## Out of scope

REQ-08 node-manipulation, multi-directional sync, on-device persistence, new ADRs.

## Next

`/architect` authors the file → `/sm` unblocks EP-014/015/020 to `draft`.
