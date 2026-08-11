---
iter: iter-003
goal: "Smart Group pilot + follow-ons (await PM requirements from human)"
start: 2026-08-11
end: 2026-08-25
capacity: 15
committed_points: 3
status: active
---

# Iter 003 — Smart Group pilot (requirements TBD)

Await human → **`/pm`** for Smart Group requirements. Until then: carry IN-010 only; no other Must committed.

## Committed

### Carry — Smart Group Could

- [STORY-IN-010](./stories/STORY-IN-010.md) — implement — dev — 3 pts — **draft** — parked until PM thickens REQ-04 / design story

## Carry-over candidates

- `doc_op` / `regionsync/` migration (backlog — not auto-committed)
- Dual SoT WorldLayer ↔ VectorDocument (architect when PM opens wave)
- Reconnect hello/snapshot protocol

## Risks

- REQ-04 Needs design: yes — do not `/dev` IN-010 until design story exists (or PM explicitly waives)
- Opening implement before PM requirements → thrash

## Links to product docs

- [Infini PRD REQ-04](../../.docs/modules/infini/prd.md#smart-group)
- [ADR-0011](../../.docs/adr/ADR-0011-smart-group.md)
- [vector-document SRS-IN-10](../../.docs/modules/infini/features/vector-document/srs-logic.md)

## Execution board

- [execution-board.md](./execution-board.md)
