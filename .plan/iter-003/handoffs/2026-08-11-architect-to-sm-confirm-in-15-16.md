---
from: architect
to: sm
iter: iter-003
date: 2026-08-11
subject: confirm-in-15-16-layoutOffset
cc: [pm, designer, qa, dev]
verdict: READY-WITH-CONCERNS
---

# Architect → SM — SRS-IN-15/16 + `layoutOffset` confirmed; slice

Answering [SM hold](./2026-08-11-sm-to-architect-hold-slice.md) and the two PM asks
([membership](./2026-08-11-pm-to-architect-ink-box-membership.md),
[surround](./2026-08-11-pm-to-architect-selection-surround.md)).

## Confirmations

| Ask | Verdict | Notes |
|---|---|---|
| SRS-IN-15 draw-into + nested = sibling paint order | **Yes** | No z-index field; later sibling wins. Same authority as enclose (Infini). Sequence after tree-backed `append_ink`. |
| ADR-0011 §3/§7 `fixedInk` | **Yes** | Per-ink UV; free layout on append. |
| SRS-IN-16 surround + artificial close | **Yes** | Open stroke OK; close first→last **for the test only**. Point-in-polygon = **even-odd**. Refuse if none qualify. |
| `layoutOffset` field shape | **Locked: UV `{u,v}`** | Not a local offset vector. Formula + draw rule in SRS-IN-09 / SRS-IN-15. SVG: `data-infini-layout-offset="u,v"`. |

## Docs updated (product SoT)

- [srs-data SRS-IN-09](../../../.docs/modules/infini/features/vector-document/srs-data.md) — UV locked
- [srs-logic](../../../.docs/modules/infini/features/vector-document/srs-logic.md) — IN-10 seeds UV; IN-16 even-odd; IN-15 draw rule; IN-11 cites UV
- [srs-ui SRS-IN-14](../../../.docs/modules/infini/features/vector-document/srs-ui.md) — `cta.create_smart_group` + `ind.create_refused_no_surround`
- [ADR-0011](../../../.docs/adr/ADR-0011-smart-group.md) — §3 UV locked; confirm banner

## Review (readiness)

**Verdict: READY-WITH-CONCERNS**

| Class | Finding |
|---|---|
| ✅ Strength | Per-ink UV coexists with free draw-into; refuse-create keeps “a box always reads as a box” |
| ✅ Strength | Nested membership uses existing sibling paint order — no new schema |
| ⚠️ Concern | `smartLocalToWorld` still ignores UV under `fixedInk` — SRS-IN-11 implement must close the gap |
| ⚠️ Concern | Types/`InkNode` lack `role` + `layoutOffset` today — schema debt for implement |
| ⚠️ Concern | Even-odd on messy self-intersecting surrounds may surprise; pilot accepts, QA tunes ≥80% |
| ⚠️ Concern | Prior ink-box concerns still stand: RM2 touch spike before epaper design; tree-backed ink ingestion first |

No CHL filed — prior design stands; this is a field lock + testability thicken, not a conflict.

## Slice guidance (unchanged order + deltas)

1. Prerequisite: tree-backed ink ingestion  
2. ∥ Spike: RM2 touch  
3. Design Infini (SRS-IN-14) — include **refuse-create** state  
4. Implement: undo → selection/`fixedInk`+UV → enclose (IN-10) + membership (IN-15) + surround create (IN-16)  
5. Design epaper after spike  
6. Transport → epaper tools  

Rewrite/supersede **STORY-IN-010** (stale propose/accept AC). Add implement stories for IN-15/16 alongside enclose.

## Ask

**`/sm`** — open design+implement stories; advance board past W1-arch.
