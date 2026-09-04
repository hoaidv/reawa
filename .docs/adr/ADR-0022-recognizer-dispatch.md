---
id: ADR-0022
title: Closure-first recognizer dispatch, one verdict per pen-up
status: accepted
date: 2026-08-14
deciders: [architect, pm]
supersedes: null
amends: [ADR-0021]
amended-by: [ADR-0038]
source: BS-0001 D15 / D20 / D21
---

# ADR-0022 — Closure-first recognizer dispatch, one verdict per pen-up

## Context

With [ADR-0021](./ADR-0021-connector-toolchip.md) both recognizers may be armed on the same
`pen` stroke — a state that was impossible when `ink_box` was an exclusive tool. Enclose wants
closed/near-closed; connector wants open with two node bindings. "Nearly disjoint" is not a spec.

[REQ-01](../modules/epaper/prd.md#local-pen-ink) ink latency must not regress. One stroke must
produce **exactly one** document verdict, logged, undoable.

## Decision

At `pen` pen-up, fall through in this fixed order. Exactly one verdict. Tool and toggle
states are those **latched at pen-down**. Closure still **gates** enclose (must be closed-ish)
and new-connector (must be open); it is not the first verdict.

| Order | When | Verdict |
|---|---|---|
| 1 | `recog.connector` armed **and** ≥80% of stroke **length** in a 5 mm world circle at **one** end of **one** existing connector | Endpoint-ink ([SRS-EP-35](../modules/epaper/features/connector-ink/srs-logic.md#srs-ep-35-endpoint-ink)) |
| 2 | Draw-into membership: ≥80% of stroke **polyline length** lies inside a SmartGroup **boundary-ink** (even-odd interior; **not** AABB sample-count) | Join that box as `role: content` ([SRS-EP-10](../modules/epaper/features/ink-box/srs-logic.md)) |
| 3 | Stroke is closed-ish **and** `recog.ink_box` armed **and** enclose guards pass | Enclose ([SRS-EP-10](../modules/epaper/features/ink-box/srs-logic.md)). If enclose **guards fail**, fall through to step 4 — do not keep the stroke as a failed enclose forever |
| 4 | Stroke is open **and** `recog.connector` armed **and** connector guards pass | Connector ([SRS-EP-17](../modules/epaper/features/connector-ink/srs-logic.md)) |
| 5 | Else | Ordinary top-level `Ink` |

Selection tools never run this table (`sel_rect` / `sel_freeform` stay pick/marquee/lasso).

Never flip a shipped outcome after it has been applied. One `[recog]` log line per pen-up:
`outcome=endpoint_ink|membership|enclose|connector|ink guard=<reason>` plus enclose-test measurements
(`fail=` `gap=` `lim=` `L=` `shorter=` `min=`) so a stayed-ink stroke shows why enclose did not fire.

Closure classifier is a deterministic geometric test on the stroke (first/last sample distance
relative to path length, plus a near-close gap cap). Exact constants live in SRS-EP-10; they
are not product vocabulary.

## Consequences

- SRS-EP-10 trigger table is no longer "`ink_box` tool → enclose else `pen` → membership".
- Draw-into membership runs **before** enclose, so a stroke that qualifies as content of an
  existing box never becomes a new box. A failed enclose may still become ordinary ink (or a
  connector if open).
- "Membership never runs on an enclose stroke" is **retired**.
- EP-016 / EP-017 must be re-verified against the new pipeline (G4 of EXP-0002).
- False-positive rate of default-on recognizers is a **ship gate** (D25), not a metric.

## Alternatives Considered

| Approach | Why rejected |
|---|---|
| Linear chain without closure class | Ambiguous stroke can hit enclose and connector; two verdicts |
| Winner-takes-all score | Non-deterministic-feeling; hard to log; flips over time |
| Connector before membership | A stroke drawn into a box would become a connector to that box |
| Enclose before membership | A closed stroke inside an existing box became a nested/competing box |
| No fall-through on failed enclose | Creator's rectangle inside a box stays orphan top-level ink |

## Amendments

Human 2026-09-05: order is **endpoint-ink → draw-into membership → enclose → new connector → ink**.
[ADR-0038](./ADR-0038-endpoint-ink-face-frame.md) names the endpoint-ink steal. Membership uses
boundary-ink polyline length, not AABB sample-count. Log `outcome=endpoint_ink` is a fifth verdict.
