---
iter: iter-003
goal: "Ink-box rework — Epaper owns the document (REQ-04…REQ-07) + Infini REQ-03 one-way sync"
start: 2026-08-11
end: 2026-08-25
capacity: 15
committed_points: 45
status: closed
---

# Iter 003 — Smart Group / ink-box pilot → document ownership rework

Pilot shipped; human verify **failed** 2026-08-11. [CHL-0008](./challenges/CHL-0008-architecture-rework.md)
**adopted 2026-08-13**. Architect design landed the same day (ADR-0014/0015, `SRS-EP-07…14`).
**NOW:** `/designer` [STORY-EP-012](./stories/STORY-EP-012.md) ∥ `/dev` [STORY-EP-013](./stories/STORY-EP-013.md)
(ink-latency measurement). No other REQ-04 implement until EP-013 passes.

[CHL-0009](./challenges/CHL-0009-missing-device-document-srs-logic.md) — `device-document/srs-logic.md`
(`SRS-EP-07` / `SRS-EP-08`) is missing; document/sync stories stay blocked until architect drops the file.

## Committed (re-slice 2026-08-13)

### W8 NOW ∥ — design + latency gate

- [STORY-EP-012](./stories/STORY-EP-012.md) — design — 5 pts — **ready** — device selection chrome (`SRS-EP-12`)
- [STORY-EP-013](./stories/STORY-EP-013.md) — implement — 3 pts — **ready** — ink latency with a resident document (`SRS-EP-13`)

### W9 — device document ∥ desktop applier

- [STORY-EP-014](./stories/STORY-EP-014.md) — implement — 5 pts — **blocked** — tree + ingestion (`SRS-EP-07`, `SRS-EP-09`) — CHL-0009 + EP-013
- [STORY-EP-015](./stories/STORY-EP-015.md) — implement — 3 pts — **blocked** — undo ring (`SRS-EP-07`) — depends EP-014
- [STORY-IN-027](./stories/STORY-IN-027.md) — implement — 5 pts — **draft** — desktop `doc_change` applier (`SRS-IN-07`)

### W10 — recognition

- [STORY-EP-016](./stories/STORY-EP-016.md) — implement — 5 pts — **draft** — enclose (`SRS-EP-10`)
- [STORY-EP-017](./stories/STORY-EP-017.md) — implement — 3 pts — **draft** — draw-into membership (`SRS-EP-10`)
- [STORY-EP-018](./stories/STORY-EP-018.md) — implement — 3 pts — **draft** — selection-create (`SRS-EP-10`) — depends EP-012

### W11 — manipulation (conformance in the same story)

- [STORY-EP-019](./stories/STORY-EP-019.md) — implement — 5 pts — **draft** — live move/resize + REQ-08 descriptor (`SRS-EP-11`, `SRS-EP-14`) — depends EP-012

### W12 — one-way sync

- [STORY-EP-020](./stories/STORY-EP-020.md) — implement — 5 pts — **blocked** — device handshake + publish (`SRS-EP-08`) — CHL-0009
- [STORY-IN-028](./stories/STORY-IN-028.md) — implement — 3 pts — **draft** — desktop handshake-gated `doc_load` (`SRS-IN-07`)

## Committed (void — pilot plan, kept as history)

Every story below assumed Infini was the sole tree writer. See the
[lifecycle map](./lifecycle-map-2026-08-13.md). Residue EP-007…011 / IN-020…026 stays **blocked**.

- IN-012, IN-014, IN-015, IN-010, IN-016, IN-017, IN-018, IN-013, EP-003, EP-004, EP-005 — **done** (pilot)
- EP-006, IN-019 — **done** (parked; do not re-verify against the old model)

## Risks

- Ink latency with a resident tree may miss ≤30 ms — EP-013 fail ⇒ `CHL-*`, stop the rework
- Missing `srs-logic.md` (CHL-0009) blocks document/sync implement
- Handle size / LOD cutoff must come from the EP-012 hardware spike, not desktop placeholders
- Capacity 15 vs 45 committed — same stretch as the pilot; W8 is the only NOW work

## Links

- epaper [REQ-04](../../.docs/modules/epaper/prd.md#device-document) ·
  [REQ-05](../../.docs/modules/epaper/prd.md#device-ink-box) ·
  [REQ-06](../../.docs/modules/epaper/prd.md#device-manipulation) ·
  [REQ-07](../../.docs/modules/epaper/prd.md#one-way-sync) · infini
  [REQ-03](../../.docs/modules/infini/prd.md#tablet-sync)
- [ADR-0014](../../.docs/adr/ADR-0014-document-ownership-inversion.md) ·
  [ADR-0015](../../.docs/adr/ADR-0015-one-way-sync-contract.md)
- [CHL-0008](./challenges/CHL-0008-architecture-rework.md) ·
  [CHL-0009](./challenges/CHL-0009-missing-device-document-srs-logic.md) ·
  [lifecycle map](./lifecycle-map-2026-08-13.md)
- [execution-board](./execution-board.md)
