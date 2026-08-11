---
from: architect
to: sm
iter: iter-002
date: 2026-08-11
subject: code-truth-srs-rewrite
cc: [pm, qa, dev]
verdict: READY-WITH-CONCERNS
---

# Architect → SM — SRS/architecture rewritten to code SoT

## Verdict: **READY-WITH-CONCERNS**

Rewrote specs so they describe **shipped** Infini↔Epaper behavior; target op-log remains
ADR-0009 long-term (amended, not replaced).

## Artifacts

| Artifact | Change |
|---|---|
| [SRS-IN-07/08](../../../.docs/modules/infini/features/tablet-sync/) | Wire: viewport+orientation+settle, doc_snapshot, stroke_*; 4 guts; marker 100 ms |
| [SRS-EP-02/03](../../../.docs/modules/epaper/features/region-sync/) | Qt SoT; gut UV; ignore region_refresh; regionsync/ = library |
| [SRS-IN-04](../../../.docs/modules/infini/features/vector-document/srs-logic.md) | Implementation status table; WorldLayer interim |
| [srs-data](../../../.docs/modules/infini/features/vector-document/srs-data.md) | `create_smart_group.children` matches code |
| [ADR-0009](../../../.docs/adr/ADR-0009-shared-document-viewport.md) | Amendment: interim wire |
| [architecture.md](../../../.docs/modules/infini/architecture.md) | Diagrams + risks updated |
| [protocol/viewport-sync.md](../../../epaper/protocol/viewport-sync.md) | Code-truth messages |
| BDD session-channels / map-append-refresh | Shipped scenarios first; `@future` / `@library` tagged |

## Design review

| Class | Finding |
|---|---|
| ✅ Strength | Same-picture rule preserved; ADR-0012 paint parity explicit |
| ⚠️ Concern | Dual SoT until tree-driven live paint |
| ⚠️ Concern | Qt path ≠ regionsync/ library — migration story needed later |
| ⚠️ Concern | Orphan SRS-IN-10 (Smart Group) expected until UI stories |

**adlc audit:** 0 orphan code; orphan SRS EP-01 / IN-05 / IN-06 / IN-10 (pre-existing / pilot).

## Ask

**Next `/sm`:** no replanning required for Must sync unless human opens migration wave.
Track W5 still awaits human hardware confirm (QA C1). Smart Group stays parked.
