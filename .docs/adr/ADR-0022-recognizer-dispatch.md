---
id: ADR-0022
title: Closure-first recognizer dispatch, one verdict per pen-up
status: accepted
date: 2026-08-14
deciders: [architect, pm]
supersedes: null
amends: [ADR-0021]
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

At `pen` pen-up, classify **closure first**, then fall through in this fixed order. Exactly
one verdict. Tool and toggle states are those **latched at pen-down**.

| Order | When | Verdict |
|---|---|---|
| 1 | Stroke is closed-ish **and** `recog.ink_box` armed | Enclose ([SRS-EP-10](../modules/epaper/features/ink-box/srs-logic.md)). If enclose **guards fail**, fall through to step 2 — do not keep the stroke as a failed enclose forever |
| 2 | Draw-into membership (≥80% samples inside a SmartGroup) | Join that box as `role: content` |
| 3 | Stroke is open **and** `recog.connector` armed **and** connector guards pass | Connector ([SRS-EP-17](../modules/epaper/features/connector-ink/srs-logic.md)) |
| 4 | Else | Ordinary top-level `Ink` |

Selection tools never run this table (`sel_rect` / `sel_freeform` stay pick/marquee/lasso).

Never flip a shipped outcome after it has been applied. One `[recog]` log line per pen-up:
`outcome=enclose|membership|connector|ink guard=<reason>`.

Closure classifier is a deterministic geometric test on the stroke (first/last sample distance
relative to path length, plus a near-close gap cap). Exact constants live in SRS-EP-10; they
are not product vocabulary.

## Consequences

- SRS-EP-10 trigger table is no longer "`ink_box` tool → enclose else `pen` → membership".
- "Membership never runs on an enclose stroke" is **retired**: a failed enclose may become
  draw-into content.
- EP-016 / EP-017 must be re-verified against the new pipeline (G4 of EXP-0002).
- False-positive rate of default-on recognizers is a **ship gate** (D25), not a metric.

## Alternatives Considered

| Approach | Why rejected |
|---|---|
| Linear chain without closure class | Ambiguous stroke can hit enclose and connector; two verdicts |
| Winner-takes-all score | Non-deterministic-feeling; hard to log; flips over time |
| Connector before membership | A stroke drawn into a box would become a connector to that box |
| No fall-through on failed enclose | Creator's rectangle inside a box stays orphan top-level ink |
