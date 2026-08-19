---
id: ADR-0027
title: Attachment parameter t on connector rest spine
status: accepted
date: 2026-08-19
deciders: [architect]
supersedes: null
amends: [ADR-0020]
source: TRACK-005 / [REQ-14]
---

# ADR-0027 — Attachment parameter t on connector rest spine

## Context

[REQ-14](../modules/epaper/prd.md#connector-attachments) hangs a node on a connection so it **stays on the line** as the line warps. [ADR-0020](./ADR-0020-connector-ink-geometry.md) I1: **never re-bake** the rest shape from a warped result. Visible connector geometry is a pure function of `{rest shape S, endpoints, warpStyle}`. Attachments must not become a second solver or a rest-shape rewrite.

Live bar: ≥5 Hz partial refresh during bound-box drag; 0 px jump on pen-up; undo of the box move restores connector **and** attachment (±1 px @ 100% zoom).

Status **accepted**: the parameterisation is forced by already-shipped ADR-0020 (pure function, no rebake). This ADR names the parameter; it does not reopen warp styles.

## Decision

An attachment is a **document node** (Text, Primitive, Ink cluster / SmartGroup, or Frame is **out** for v1 hang) plus a **binding** on the connector:

```text
Connector.attachments[] = { nodeId, t, offset: { d } }
t ∈ [0, 1]  // normalized arc-length on rest spine S, never on warped V
d           // signed perpendicular offset in world units at bind time (may be 0)
```

| Rule | Value |
|---|---|
| Bind-time `t` | Arc-length parameter of the nearest point on **S** to the node’s AABB centroid (or explicit place point) |
| Pose later | World position = point on **warped** spine `V` at the same `t`, plus rotate `d` into `V`’s local normal at `t` |
| Rebake | **Forbidden.** Moving a bound box re-warps `V` and **re-places** attachments; `S` and stored `t` do not change |
| Box move op | Still only `set_smart_transform` on the box ([ADR-0020](./ADR-0020-connector-ink-geometry.md) § Consequences). Attachments are derived at paint/hit — **0** extra ops per frame |
| Commit | On pen-up, attachment pose equals last preview (0 px jump). Optional persist of derived world cache is not an op (same class as connector last-live pose) |
| Undo box move | Snapshot ring restores box + derived connector + derived attachment poses |
| Connector with 0 attachments | REQ-09 warp unchanged (0 regression) |

v1 does not attach to the cubic `C` parameter or to screen pixels.

## Consequences

- Domain anatomy: [vector-document](../domain/vector-document.md) Connector attachments.
- Behaviour: [SRS-EP-38](../modules/epaper/features/connector-ink/srs-logic.md).
- Manual attach ([REQ-17](../modules/epaper/prd.md#manual-create)) writes the same `{nodeId, t, d}` — no second model.
- Sensitivity: **`t` on S vs `t` on V**. Parameterising on `V` would drift as Morph mix `m` grows and would pressure a rebake. Rest-spine `t` is the trade-off that keeps I1.

## Alternatives Considered

| Approach | Stays on line | ADR-0020 I1 | Live cost | Why |
|---|---|---|---|---|
| Rebake S each drag so attachments are “on the new rest” | 0 | − | − | Rejected — EXP proved tens of units of drift |
| Parent attachments in connector local space as children | + | 0 | 0 | Rejected — ADR-0020: connector is **not** a spatial parent; Infini paint/parenting would fork |
| `t` on warped `V` | + at one pose | − (pose-dependent) | 0 | Rejected — not a pure function of rest+ends |
| **`t` on S, place from V (this ADR)** | + | + | + | Winner — forced by shipped warp |

Trade-off point: **attachment glued to ink identity (S)** vs **glued to current pixels (V)**. Identity of the parameter wins; pixels are derived.
