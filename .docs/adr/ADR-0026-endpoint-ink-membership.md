---
id: ADR-0026
title: Endpoint-ink membership (end vs spine vs empty)
status: proposed
date: 2026-08-19
deciders: [architect, pm]
supersedes: null
amends: [ADR-0022]
source: TRACK-005 / [REQ-13]
---

# ADR-0026 — Endpoint-ink membership (end vs spine vs empty)

## Context

[REQ-13](../modules/epaper/prd.md#connector-ends) Path B: strokes whose samples run over a connector **end** become **part of that end**, not ordinary ink and not a second connector; they ride warp; wrong recog costs one undo. The **same** stroke over empty canvas or the connector **spine** must **not** be stolen.

[ADR-0022](./ADR-0022-recognizer-dispatch.md) currently ends in ordinary `Ink` (step 4) after enclose / membership / new-connector. Without a named membership test, implementers will either AABB-steal the whole connector (false endpoint style) or never bind decoration (REQ miss).

[ADR-0020](./ADR-0020-connector-ink-geometry.md) reserved a `terminal` slot per end. Rest spine `S` is never rebaked.

## Decision

Insert a **deterministic geometric test** into the pen-up dispatch **after** draw-into membership and **before** new-connector recognition.

### End vs spine vs empty

Let `C` be an existing connector. Rest spine `S` has normalized arc-length `s ∈ [0,1]`.

| Region | Definition (v1, closed) |
|---|---|
| **End** (start) | Samples whose nearest point on **warped** spine `V` has `s ≤ 0.08` **or** world distance to `V[0]` ≤ `R_END` |
| **End** (finish) | `s ≥ 0.92` **or** distance to `V[last]` ≤ `R_END` |
| **Spine** | Nearest `s ∈ (0.08, 0.92)` and distance to `V` ≤ `R_SPINE`, and **not** in an End |
| **Empty** | Neither End nor Spine of any connector |

`R_END = max(24 u, 2 × world stroke width of C)`. `R_SPINE = max(8 u, 1 × world stroke width of C)`.

### Steal rule

| Condition | Verdict |
|---|---|
| ≥80% of the new stroke’s samples lie in **one** end region of **one** connector | Bind as **endpoint decoration** of that end. **0** new free Ink at that location. **0** second connector. Log `[recog] outcome=endpoint_ink end=start\|finish id=<connectorId>` |
| Samples in two different ends (same or different connectors), or End+Spine mixed so neither end has ≥80% | **Do not steal** — fall through (connector / ordinary ink) |
| ≥80% in Spine (not End) | **Do not steal** — ordinary ink / membership / connector rules apply |
| Empty canvas | **Do not steal** |

Decoration is stored on the connector’s `terminal[end].ink` (polyline in the **end frame**, like `drawnEdgeLocal`): samples projected to `(s, d)` against **rest** spine `S` at bind time. Warp **re-places** them with the end (they ride). Rest shape `S` is **not** rebuilt ([ADR-0020](./ADR-0020-connector-ink-geometry.md) I1).

Wrong bind: one undo restores pre-stroke document (decoration gone, stroke back as it would have been — v1 restores the snapshot, same as other recog).

Toolbar Path A (pick a closed style) is **independent**: it sets `terminal[end].style` without requiring ink.

## Consequences

- Amends ADR-0022 order: `1 enclose → 2 membership → 2.5 endpoint-ink → 3 new connector → 4 ink`.
- [SRS-EP-35](../modules/epaper/features/connector-ink/srs-logic.md) owns the test; [SRS-EP-17](../modules/epaper/features/connector-ink/srs-logic.md) stays new-connector recognition.
- False-positive ship gate ([SRS-EP-20](../modules/epaper/features/connector-ink/srs-quality.md) ≤2%) **includes** unintended endpoint-ink binds. Tight `s` caps (0.08 / 0.92) are the sensitivity point.
- [SRS-EP-19](../modules/epaper/features/connector-ink/srs-ui.md) “Out of scope: Arrowheads” is superseded for REQ-13 by [SRS-EP-36](../modules/epaper/features/connector-ink/srs-ui.md) — see [CHL-0022](../../.plan/iter-005/challenges/CHL-0022-shipped-no-device-pan.md) (same challenge file also lists this chrome out-of-scope conflict).

## Alternatives Considered

| Approach | Spine safety | Warp-ride | Dispatch simplicity | Why |
|---|---|---|---|---|
| AABB of whole connector | − | 0 | + | Rejected — steals spine ink as an “end” |
| New exclusive “endpoint” tool | + | + | − | Rejected — PRD Path B is recognition, not a fifth tool |
| Bind to warped `V` and rebake `S` | 0 | − | − | Rejected — violates ADR-0020 I1 |
| **End-cap s-window + R_END (this ADR)** | + | + | 0 | Winner |

Trade-off point: **recall of decorative ticks on the arrowhead** vs **not stealing handwriting on the shaft**. Caps are tight by default; QA may file a CHL to retune `0.08` / `R_END` in du, not to change the region kinds.
